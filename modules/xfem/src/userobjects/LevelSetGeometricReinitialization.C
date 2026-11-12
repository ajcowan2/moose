//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "LevelSetGeometricReinitialization.h"
#include "FEProblemBase.h"
#include "MooseMesh.h"
#include "MooseVariableFieldBase.h"

#include "libmesh/elem.h"
#include "libmesh/node.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

namespace
{
/// Append a debug message to the per-rank file xfem_debug_rank<N>.log (opened once)
void
dbgEmitReinit(const unsigned int rank, const std::string & msg)
{
  static std::ofstream f;
  if (!f.is_open())
    f.open("xfem_debug_rank" + std::to_string(rank) + ".log", std::ios::app);
  f << msg << std::flush;
}

/// Format a number in compact scientific notation for the debug output
std::string
dbgFmtReinit(const Real v)
{
  std::ostringstream o;
  o << std::scientific << std::setprecision(4) << v;
  return o.str();
}

/// Distance from point p to the segment a-b
Real
distanceToSegment(const Real px, const Real py, const Real * s)
{
  const Real dx = s[2] - s[0];
  const Real dy = s[3] - s[1];
  const Real len2 = dx * dx + dy * dy;
  // Parameter of the closest point on the segment, clamped to [0, 1]; a zero-length segment is a point
  Real t = len2 > 0 ? ((px - s[0]) * dx + (py - s[1]) * dy) / len2 : 0.0;
  t = std::min(1.0, std::max(0.0, t));
  return std::hypot(px - (s[0] + t * dx), py - (s[1] + t * dy));
}
}

registerMooseObject("XFEMApp", LevelSetGeometricReinitialization);

InputParameters
LevelSetGeometricReinitialization::validParams()
{
  InputParameters params = GeneralUserObject::validParams();
  params.addClassDescription(
      "Reinitializes a level set variable as the exact signed distance to its piecewise linear "
      "zero contour (the contour LevelSetCutUserObject cuts along). Does not move the contour "
      "and needs no pseudo-time solve.");
  params.addRequiredParam<VariableName>("variable",
                                        "The first order nodal level set variable to reinitialize");
  params.addParam<bool>("debug",
                        false,
                        "Print one summary line per reinitialization (contour segments, cut and "
                        "ambiguous elements, largest change of the level set, sign changes) to "
                        "the console, and the largest change with its location per rank to "
                        "xfem_debug_rank<N>.log.");
  params.addParam<unsigned int>(
      "debug_interval",
      1,
      "With debug = true, only print at time steps that are a multiple of this number.");
  // Reinitialize at the end of each step, after the level set solve and before the next step
  params.set<ExecFlagEnum>("execute_on") = EXEC_TIMESTEP_END;
  return params;
}

LevelSetGeometricReinitialization::LevelSetGeometricReinitialization(
    const InputParameters & parameters)
  : GeneralUserObject(parameters),
    _system(_fe_problem.getSystem(getParam<VariableName>("variable"))),
    _var_number(_fe_problem
                    .getVariable(_tid,
                                 getParam<VariableName>("variable"),
                                 Moose::VarKindType::VAR_ANY,
                                 Moose::VarFieldType::VAR_FIELD_STANDARD)
                    .number()),
    _debug(getParam<bool>("debug")),
    _debug_interval(std::max(1u, getParam<unsigned int>("debug_interval")))
{
  if (_debug)
    _console << "[DBG-INIT] " << type() << " '" << name()
             << "': debug output on; per-rank files xfem_debug_rank<N>.log are written in "
             << std::filesystem::current_path().string() << std::endl;
  if (_fe_problem.mesh().getMesh().mesh_dimension() != 2)
    mooseError("LevelSetGeometricReinitialization is implemented for 2D meshes only.");
}

void
LevelSetGeometricReinitialization::execute()
{
  const MeshBase & mesh = _fe_problem.mesh().getMesh();
  const unsigned int sys_num = _system.number();
  const NumericVector<Number> & ls_old = *_system.current_local_solution;

  const auto ls_at = [&](const Node & n) { return ls_old(n.dof_number(sys_num, _var_number, 0)); };

  // 1. Zero contour of the local elements as segments (x1 y1 x2 y2), same crossings as the XFEM cut
  std::vector<Real> segments;
  dof_id_type n_cut = 0, n_ambiguous = 0, n_skipped = 0;
  for (const auto * elem : mesh.active_local_element_ptr_range())
  {
    std::vector<Point> pts;
    for (const auto s : make_range(elem->n_sides()))
    {
      // Only the two vertices of a side: higher order mid-side nodes do not define the contour
      const Node & n1 = *elem->node_ptr(elem->local_side_node(s, 0));
      const Node & n2 = *elem->node_ptr(elem->local_side_node(s, 1));
      const Real l1 = ls_at(n1), l2 = ls_at(n2);
      if (l1 * l2 < 0)
        pts.push_back(Point(n1) + std::abs(l1) / std::abs(l1 - l2) * (Point(n2) - Point(n1)));
    }
    // A node exactly on the front is a point of the contour that no edge test above sees
    for (const auto v : make_range(elem->n_vertices()))
      if (ls_at(*elem->node_ptr(v)) == 0)
        pts.push_back(*elem->node_ptr(v));

    if (pts.empty())
      continue;
    ++n_cut;
    // Two points: one segment. Four (saddle): pair them in side order. Anything else (the front only
    // touches the element at one node) has no segment inside this element.
    if (pts.size() != 2 && pts.size() != 4)
    {
      ++n_skipped;
      continue;
    }
    n_ambiguous += pts.size() == 4;
    for (std::size_t i = 0; i < pts.size(); i += 2)
      segments.insert(segments.end(), {pts[i](0), pts[i](1), pts[i + 1](0), pts[i + 1](1)});
  }
  _communicator.allgather(segments);
  _communicator.sum(n_cut);
  _communicator.sum(n_ambiguous);
  _communicator.sum(n_skipped);

  const std::size_t n_segments = segments.size() / 4;
  const bool print = _debug && _t_step % _debug_interval == 0;
  if (n_segments == 0)
  {
    // No front in the domain: keep the field (nothing to measure the distance to)
    if (print && processor_id() == 0)
      _console << "[DBG-REINIT] t_step=" << _t_step << " no zero contour found, level set unchanged"
               << std::endl;
    return;
  }

  // 2. Signed distance at the local nodes, written into the solution of the level set variable
  Real max_change = 0.0;
  Point max_change_at;
  dof_id_type n_nodes = 0, n_sign_zero = 0;
  for (const auto * node : mesh.local_node_ptr_range())
  {
    if (node->n_dofs(sys_num, _var_number) == 0)
      continue;
    const dof_id_type dof = node->dof_number(sys_num, _var_number, 0);
    const Real old_value = ls_old(dof);

    Real dist = std::numeric_limits<Real>::max();
    for (const auto i : make_range(n_segments))
      dist = std::min(dist, distanceToSegment((*node)(0), (*node)(1), &segments[4 * i]));

    // Keep the old side; a value of exactly 0 stays exactly 0 (the front passes through the node)
    const Real sign = old_value > 0 ? 1.0 : (old_value < 0 ? -1.0 : 0.0);
    n_sign_zero += sign == 0.0;
    const Real new_value = sign * dist;
    if (std::abs(new_value - old_value) > max_change)
    {
      max_change = std::abs(new_value - old_value);
      max_change_at = *node;
    }
    _system.solution->set(dof, new_value);
    ++n_nodes;
  }
  _system.solution->close();
  _system.update(); // refresh the ghosted current_local_solution that the next step reads
  _communicator.sum(n_nodes);
  _communicator.sum(n_sign_zero);

  // Debug: per-rank detail, then the global summary from rank 0
  if (print)
  {
    std::ostringstream dbg;
    dbg << "[rank " << processor_id() << "] [DBG-REINIT] t_step=" << _t_step
        << " t=" << dbgFmtReinit(_t) << " local max |ls_new - ls_old|=" << dbgFmtReinit(max_change)
        << " at (" << dbgFmtReinit(max_change_at(0)) << "," << dbgFmtReinit(max_change_at(1))
        << ")\n";
    dbgEmitReinit(processor_id(), dbg.str());
  }
  Real global_max_change = max_change;
  _communicator.max(global_max_change);
  if (print && processor_id() == 0)
    _console << "[DBG-REINIT] t_step=" << _t_step << " t=" << dbgFmtReinit(_t)
             << " segments=" << n_segments << " cut_elems=" << n_cut
             << " ambiguous_4_crossing=" << n_ambiguous << " skipped_elems=" << n_skipped
             << " nodes=" << n_nodes << " nodes_at_ls=0=" << n_sign_zero
             << " max|ls_new-ls_old|=" << dbgFmtReinit(global_max_change) << std::endl;
}
