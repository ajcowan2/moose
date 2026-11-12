//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "XFEMEqualValueAtInterface.h"
#include "FEProblem.h"
#include "GeometricCutUserObject.h"
#include "XFEM.h"
#include "AuxiliarySystem.h"
#include "Assembly.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace
{
/// Append a debug message to the per-rank file xfem_debug_rank<N>.log (opened once)
void
dbgEmitCon(const unsigned int rank, const std::string & msg)
{
  static std::ofstream f;
  if (!f.is_open())
    f.open("xfem_debug_rank" + std::to_string(rank) + ".log", std::ios::app);
  f << msg << std::flush;
}

/// Format a number in compact scientific notation for the debug output
std::string
dbgFmtCon(const Real v)
{
  std::ostringstream o;
  o << std::scientific << std::setprecision(4) << v;
  return o.str();
}
}

registerMooseObject("XFEMApp", XFEMEqualValueAtInterface);

InputParameters
XFEMEqualValueAtInterface::validParams()
{
  InputParameters params = ElemElemConstraint::validParams();
  params.addRequiredParam<Real>("alpha", "Penalty parameter in penalty formulation.");
  params.addRequiredParam<Real>("value", "Prescribed value at the interface.");
  params.addRequiredParam<Real>("value_neighbor", "Prescribed value at the interface.");
  params.addRequiredParam<bool>("use_penalty", "Use penalty approach.");
  params.addParam<Real>(
      "diff", 1., "The diffusion (or thermal conductivity or viscosity) coefficient.");
  params.addParam<PostprocessorName>(
      "diffusivity_positive",
      "Diffusivity on the positive level set side. Must be given together with "
      "diffusivity_negative; the pair replaces diff, so that the flux, adjoint and penalty terms "
      "of each side use the diffusivity of that side.");
  params.addParam<PostprocessorName>("diffusivity_negative",
                                     "Diffusivity on the negative level set side.");
  MooseEnum penalty_scaling("sqrt linear", "sqrt");
  params.addParam<MooseEnum>(
      "penalty_scaling",
      penalty_scaling,
      "Scale factor C of the penalty term alpha * (u - g) * test * C. sqrt: sqrt(D area / vol) "
      "(original). linear: D area / vol, which has the dimension of the Nitsche penalty D / h so "
      "that alpha is O(1) to O(10) independent of D.");
  params.addParam<bool>("rz_interface_weighting",
                        false,
                        "Multiply the interface quadrature weights by the coordinate transformation "
                        "factor of the mesh (2 pi r for coord_type = RZ, 1 for Cartesian), the same "
                        "factor MOOSE applies to volume and boundary integrals. Without it the "
                        "flux, adjoint and penalty terms are 1 / (2 pi r) times too strong relative "
                        "to the bulk terms in an axisymmetric model.");
  params.addRequiredParam<VariableName>(
      "level_set_var", "The name of level set variable used to represent the interface");
  params.addParam<UserObjectName>(
      "geometric_cut_userobject",
      "Name of GeometricCutUserObject associated with this constraint.");
  params.addParam<bool>("debug",
                        false,
                        "Print per-element-pair diagnostics at every residual evaluation: "
                        "volume fractions, cut area, side selection, interface values, the "
                        "value mismatch (u - prescribed), normal fluxes and the size of the "
                        "flux, adjoint and penalty residual terms.");
  params.addParam<unsigned int>(
      "debug_interval",
      1,
      "With debug = true, only write the records of time steps that are a multiple of this.");
  params.addParam<Real>("debug_radius",
                        -1.0,
                        "With debug = true, only print element pairs whose current element "
                        "centroid is within this distance of debug_point; negative prints all.");
  params.addParam<Point>("debug_point", Point(), "Center of the debug printing window.");
  params.addClassDescription("Enforce that the solution have the same value on opposing sides of "
                             "an XFEM interface.");
  params.addRelationshipManager("ElementSideNeighborLayers",
                                Moose::RelationshipManagerType::ALGEBRAIC,
                                [](const InputParameters &, InputParameters & rm_params)
                                { rm_params.set<unsigned short>("layers") = 2; });
  return params;
}

XFEMEqualValueAtInterface::XFEMEqualValueAtInterface(const InputParameters & parameters)
  : ElemElemConstraint(parameters),
    _alpha(getParam<Real>("alpha")),
    _value(getParam<Real>("value")),
    _value_neighbor(getParam<Real>("value_neighbor")),
    _level_set_var_number(_subproblem
                              .getVariable(_tid,
                                           parameters.get<VariableName>("level_set_var"),
                                           Moose::VarKindType::VAR_ANY,
                                           Moose::VarFieldType::VAR_FIELD_STANDARD)
                              .number()),
    _system(_subproblem.getSystem(getParam<VariableName>("level_set_var"))),
    _solution(*_system.current_local_solution.get()),
    _use_penalty(getParam<bool>("use_penalty")),
    _diff(getParam<Real>("diff")),
    _diff_positive(isParamValid("diffusivity_positive")
                       ? &getPostprocessorValue("diffusivity_positive")
                       : nullptr),
    _diff_negative(isParamValid("diffusivity_negative")
                       ? &getPostprocessorValue("diffusivity_negative")
                       : nullptr),
    _linear_penalty(getParam<MooseEnum>("penalty_scaling") == "linear"),
    _rz_interface_weighting(getParam<bool>("rz_interface_weighting")),
    _debug(getParam<bool>("debug")),
    _debug_interval(std::max(1u, getParam<unsigned int>("debug_interval"))),
    _debug_radius(getParam<Real>("debug_radius")),
    _debug_point(getParam<Point>("debug_point"))
{
  if (_debug)
    _console << "[DBG-INIT] " << type() << " '" << name()
             << "': debug output on; per-rank files xfem_debug_rank<N>.log are written in "
             << std::filesystem::current_path().string() << std::endl;
  if ((_diff_positive == nullptr) != (_diff_negative == nullptr))
    mooseError("XFEMEqualValueAtInterface: diffusivity_positive and diffusivity_negative must be "
               "given together.");

  _xfem = std::dynamic_pointer_cast<XFEM>(_fe_problem.getXFEM());
  if (_xfem == nullptr)
    mooseError("Problem casting to XFEM in XFEMEqualValueAtInterface");

  const UserObject * uo =
      &(_fe_problem.getUserObjectBase(getParam<UserObjectName>("geometric_cut_userobject")));

  if (dynamic_cast<const GeometricCutUserObject *>(uo) == nullptr)
    mooseError("UserObject casting to GeometricCutUserObject in XFEMEqualValueAtInterface");

  _interface_id = _xfem->getGeometricCutID(dynamic_cast<const GeometricCutUserObject *>(uo));
}

XFEMEqualValueAtInterface::~XFEMEqualValueAtInterface() {}

void
XFEMEqualValueAtInterface::reinitConstraintQuadrature(const ElementPairInfo & element_pair_info)
{
  _interface_normal = element_pair_info._elem1_normal;
  ElemElemConstraint::reinitConstraintQuadrature(element_pair_info);

  // The cut line / surface weights from XFEM do not include the coordinate transformation factor
  // that MOOSE applies to the bulk (JxW) and boundary integrals. The penalty scale factor uses
  // area / vol, where the factor cancels, so it is left unchanged.
  if (_rz_interface_weighting)
    for (const auto qp : index_range(_constraint_weight))
    {
      Real coord = 1.0;
      coordTransformFactor(
          _subproblem, _current_elem->subdomain_id(), _constraint_q_point[qp], coord);
      _constraint_weight[qp] *= coord;
    }
}

void
XFEMEqualValueAtInterface::debugFlush()
{
  std::ostringstream dbg;
  for (const auto & [elem_id, rec] : _dbg_last)
  {
    if (rec.step % _debug_interval != 0)
      continue;
    dbg << "[rank " << processor_id() << "] [DBG-CON] t_step=" << rec.step
        << " t=" << dbgFmtCon(rec.t) << " elem=" << elem_id << " neighbor=" << rec.neighbor
        << " centroid=(" << dbgFmtCon(rec.cx) << "," << dbgFmtCon(rec.cy) << ")"
        << " | vol_frac(elem,neighbor)=" << dbgFmtCon(rec.vf_elem) << ","
        << dbgFmtCon(rec.vf_neigh) << " cut_area=" << dbgFmtCon(rec.area)
        << " | level set nodes of neighbors (pos,neg)=" << rec.count_pos << "," << rec.count_neg
        << " use_positive_property=" << rec.use_positive
        << " | u(elem,neighbor)=" << dbgFmtCon(rec.u_elem) << "," << dbgFmtCon(rec.u_neigh)
        << " mismatch u-g(elem,neighbor)=" << dbgFmtCon(rec.mismatch_elem) << ","
        << dbgFmtCon(rec.mismatch_neigh) << " | grad_u.n(elem,neighbor)="
        << dbgFmtCon(rec.gradn_elem) << "," << dbgFmtCon(rec.gradn_neigh)
        << " | diff(elem,neighbor)=" << dbgFmtCon(rec.D_elem) << "," << dbgFmtCon(rec.D_neigh)
        << " C(elem,neighbor)=" << dbgFmtCon(rec.C_elem) << "," << dbgFmtCon(rec.C_neigh)
        << " | residual terms (elem, test 0): flux=" << dbgFmtCon(rec.flux_term)
        << " adjoint=" << dbgFmtCon(rec.adjoint_term) << " penalty=" << dbgFmtCon(rec.penalty_term)
        << "\n";
  }
  if (!dbg.str().empty())
    dbgEmitCon(processor_id(), dbg.str());
  _dbg_last.clear();
}

bool
XFEMEqualValueAtInterface::elemOnPositiveSide(unsigned int & count_pos,
                                              unsigned int & count_neg) const
{
  count_pos = 0;
  count_neg = 0;

  const std::set<unsigned int> new_nodes = _xfem->getNewNodes();

  for (auto neighbor : _current_elem->neighbor_ptr_range())
    if (neighbor != nullptr)
      for (const auto i : neighbor->node_index_range())
      {
        const Node * node = neighbor->node_ptr(i);
        if (new_nodes.find(node->id()) == new_nodes.end())
        {
          const dof_id_type ls_dof_id = node->dof_number(_system.number(), _level_set_var_number, 0);
          // The interface is the zero level set, matching LevelSetCutUserObject
          if (_solution(ls_dof_id) >= 0.0)
            ++count_pos;
          else
            ++count_neg;
        }
      }

  return count_pos > count_neg;
}

Real
XFEMEqualValueAtInterface::penaltyFactor(const Real D,
                                         const Real area,
                                         const Real phys_vol) const
{
  const Real ratio = std::abs(D * area / phys_vol);
  return _linear_penalty ? ratio : std::sqrt(ratio);
}

Real
XFEMEqualValueAtInterface::computeQpResidual(Moose::DGResidualType type)
{
  Real area = _xfem->getCutPlaneArea(_current_elem);
  Real elem_vol = _xfem->getPhysicalVolumeFraction(_current_elem) * _current_elem->volume();
  Real neighbor_vol = _xfem->getPhysicalVolumeFraction(_neighbor_elem) * _neighbor_elem->volume();

  unsigned int count_pos = 0;
  unsigned int count_neg = 0;

  const std::set<unsigned int> new_nodes = _xfem->getNewNodes();

  std::set<unsigned int>::const_iterator it;

  for (auto neighbor : _current_elem->neighbor_ptr_range())
  {
    if (neighbor != nullptr)
      for (unsigned int i = 0; i < neighbor->n_nodes(); i++)
      {
        const Node * node = neighbor->node_ptr(i);
        if (new_nodes.find(node->id()) == new_nodes.end())
        {
          // std::cout << "node id = " << node->id() << std::endl;
          // std::cout << "node = " << *node << std::endl;
          // std::cout << "_level_set_var_number = " << _level_set_var_number << std::endl;
          // std::cout << "_system number = " << _system.number() << std::endl;
          // neighbor->print_info();
          // node->print_dof_info();
          dof_id_type ls_dof_id = node->dof_number(_system.number(), _level_set_var_number, 0);
          Number ls_node_value = _solution(ls_dof_id);
          // The interface is the zero level set, matching LevelSetCutUserObject
          if (ls_node_value >= 0.0)
            count_pos += 1;
          else
            count_neg += 1;
        }
      }
  }

  // for (unsigned int i = 0; i < _current_elem->n_nodes(); i++)
  // {
  //   const Node * node = _current_elem->node_ptr(i);
  //
  //   dof_id_type ls_dof_id = node->dof_number(_system.number(), _level_set_var_number, 0);
  //   Number ls_node_value = _solution(ls_dof_id);
  //
  //   if (_xfem->isPointInsidePhysicalDomain(_current_elem, *node))
  //   {
  //     if (ls_node_value >= 0.5)
  //       count_pos += 1;
  //   }
  //   else
  //   {
  //     if (ls_node_value < 0.5)
  //       count_pos += 1;
  //   }
  // }

  Real use_positive_property = false;

  // std::cout << "area = " << area << ", elem_vol = " << elem_vol
  //           << ", neighbor_vol =  " << neighbor_vol << std::endl;

  // std::cout << "count_pos = " << count_pos << std::endl;

  // if (count_pos / _current_elem->n_nodes() > 0.8)
  //   use_positive_property = true;

  if (count_pos > count_neg)
    use_positive_property = true;

  // const Node * node = _current_elem->node_ptr(0);
  // dof_id_type ls_dof_id = node->dof_number(_system.number(), _level_set_var_number, 0);
  // Number ls_node_value = _solution(ls_dof_id);

  // if (_xfem->isPointInsidePhysicalDomain(_current_elem, *node))
  // {
  //   if (ls_node_value >= 0.5)
  //     use_positive_property = true;
  // }
  // else
  // {
  //   if (ls_node_value < 0.5)
  //     use_positive_property = true;
  // }

  Real r = 0;

  // Diffusivity of the side of the element and of its neighbor
  const bool two_diffs = _diff_positive && _diff_negative;
  const Real D_elem = two_diffs ? (use_positive_property ? *_diff_positive : *_diff_negative)
                                : _diff;
  const Real D_neigh = two_diffs ? (use_positive_property ? *_diff_negative : *_diff_positive)
                                 : _diff;

  Real C_elem = penaltyFactor(D_elem, area, elem_vol);
  Real C_neigh = penaltyFactor(D_neigh, area, neighbor_vol);

  // C_elem = std::min(C_elem, 1e5);
  // C_neigh = std::min(C_neigh, 1e5);

  // C_elem = 1.0;
  // C_neigh = 1.0;

  // Debug: remember the latest residual evaluation of each cut element pair; the records of a
  // time step are formatted and written when the next time step starts (no string work here)
  if (_debug)
  {
    if (_fe_problem.timeStep() != _dbg_step)
    {
      debugFlush();
      _dbg_step = _fe_problem.timeStep();
    }
    if (type == Moose::Element && _i == 0 && _qp == 0 &&
        (_debug_radius < 0 ||
         (_current_elem->vertex_average() - _debug_point).norm() <= _debug_radius))
    {
      const Real g_elem = use_positive_property ? _value : _value_neighbor;
      const Real g_neigh = use_positive_property ? _value_neighbor : _value;
      const Point c = _current_elem->vertex_average();
      DebugRecord & rec = _dbg_last[_current_elem->id()];
      rec.step = _fe_problem.timeStep();
      rec.t = _fe_problem.time();
      rec.neighbor = _neighbor_elem->id();
      rec.cx = c(0);
      rec.cy = c(1);
      rec.vf_elem = _xfem->getPhysicalVolumeFraction(_current_elem);
      rec.vf_neigh = _xfem->getPhysicalVolumeFraction(_neighbor_elem);
      rec.area = area;
      rec.count_pos = count_pos;
      rec.count_neg = count_neg;
      rec.use_positive = use_positive_property;
      rec.u_elem = _u[_qp];
      rec.u_neigh = _u_neighbor[_qp];
      rec.mismatch_elem = _u[_qp] - g_elem;
      rec.mismatch_neigh = _u_neighbor[_qp] - g_neigh;
      rec.gradn_elem = _grad_u[_qp] * _interface_normal;
      rec.gradn_neigh = _grad_u_neighbor[_qp] * -_interface_normal;
      rec.D_elem = D_elem;
      rec.D_neigh = D_neigh;
      rec.C_elem = C_elem;
      rec.C_neigh = C_neigh;
      rec.flux_term = -_test[_i][_qp] * (_grad_u[_qp] * D_elem * _interface_normal);
      rec.adjoint_term =
          (_u[_qp] - g_elem) * (_grad_test[_i][_qp] * D_elem * _interface_normal);
      rec.penalty_term = _alpha * (_u[_qp] - g_elem) * _test[_i][_qp] * C_elem;
    }
  }

  // std::cout << "_u[_qp] = " << _u[_qp] << ", _u_neighbor[_qp] = " << _u_neighbor[_qp]
  //           << ", area = " << area << std::endl;

  switch (type)
  {
    case Moose::Element:

      if (!_use_penalty)
      {
        r += -_test[_i][_qp] * (_grad_u[_qp] * D_elem * _interface_normal) +
             (_u[_qp] - (use_positive_property ? _value : _value_neighbor)) *
                 (_grad_test[_i][_qp] * D_elem * _interface_normal);
        r += _alpha * (_u[_qp] - (use_positive_property ? _value : _value_neighbor)) *
             _test[_i][_qp] * C_elem;
      }
      else
        r += _alpha * (_u[_qp] - (use_positive_property ? _value : _value_neighbor)) *
             _test[_i][_qp] * C_elem;
      break;

    case Moose::Neighbor:
      if (!_use_penalty)
      {
        r += -_test_neighbor[_i][_qp] * (_grad_u_neighbor[_qp] * D_neigh * -_interface_normal) +
             (_u_neighbor[_qp] - (use_positive_property ? _value_neighbor : _value)) *
                 (_grad_test_neighbor[_i][_qp] * D_neigh * -_interface_normal);
        r += _alpha * (_u_neighbor[_qp] - (use_positive_property ? _value_neighbor : _value)) *
             _test_neighbor[_i][_qp] * C_neigh;
      }
      else
        r += _alpha * (_u_neighbor[_qp] - (use_positive_property ? _value_neighbor : _value)) *
             _test_neighbor[_i][_qp] * C_neigh;
      break;
  }
  return r;
}

Real
XFEMEqualValueAtInterface::computeQpJacobian(Moose::DGJacobianType type)
{
  Real area = _xfem->getCutPlaneArea(_current_elem);
  Real elem_vol = _xfem->getPhysicalVolumeFraction(_current_elem) * _current_elem->volume();
  Real neighbor_vol = _xfem->getPhysicalVolumeFraction(_neighbor_elem) * _neighbor_elem->volume();

  Real r = 0;

  unsigned int count_pos = 0, count_neg = 0;
  const bool two_diffs = _diff_positive && _diff_negative;
  const bool elem_positive = two_diffs ? elemOnPositiveSide(count_pos, count_neg) : false;
  const Real D_elem = two_diffs ? (elem_positive ? *_diff_positive : *_diff_negative) : _diff;
  const Real D_neigh = two_diffs ? (elem_positive ? *_diff_negative : *_diff_positive) : _diff;

  Real C_elem = penaltyFactor(D_elem, area, elem_vol);
  Real C_neigh = penaltyFactor(D_neigh, area, neighbor_vol);
  // C_elem = std::min(C_elem, 1e5);
  // C_neigh = std::min(C_neigh, 1e5);
  // C_elem = 1.0;
  // C_neigh = 1.0;

  switch (type)
  {
    case Moose::ElementElement:
      if (!_use_penalty)
      {
        r += -_test[_i][_qp] * (_grad_phi[_j][_qp] * D_elem * _interface_normal) +
             _phi[_j][_qp] * (_grad_test[_i][_qp] * D_elem * _interface_normal);
        r += _alpha * _phi[_j][_qp] * _test[_i][_qp] * C_elem;
      }
      else
        r += _alpha * _phi[_j][_qp] * _test[_i][_qp] * C_elem;
      break;

    case Moose::NeighborNeighbor:
      if (!_use_penalty)
      {
        r += -_test_neighbor[_i][_qp] * (_grad_phi_neighbor[_j][_qp] * D_neigh * -_interface_normal) +
             _phi_neighbor[_j][_qp] * (_grad_test_neighbor[_i][_qp] * D_neigh * -_interface_normal);
        r += _alpha * _phi_neighbor[_j][_qp] * _test_neighbor[_i][_qp] * C_neigh;
      }
      else
        r += _alpha * _phi_neighbor[_j][_qp] * _test_neighbor[_i][_qp] * C_neigh;
      break;

    default:
      break;
  }

  return r;
}
