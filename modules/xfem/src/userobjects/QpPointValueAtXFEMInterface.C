//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "QpPointValueAtXFEMInterface.h"
#include "MooseVariableFE.h"
#include "XFEM.h"
#include "GeometricCutUserObject.h"
#include "libmesh/parallel_algebra.h"
#include "libmesh/parallel.h"
#include <set>
#include <sstream>
#include <limits>
#include <iomanip>

registerMooseObject("XFEMApp", QpPointValueAtXFEMInterface);

InputParameters
QpPointValueAtXFEMInterface::validParams()
{
  InputParameters params = GeneralUserObject::validParams();
  params.addRequiredParam<VariableName>(
      "variable", "The name of the variable that this UserObject operates on");
  params.addParam<UserObjectName>(
      "interface_mesh_cut_userobject",
      "Name of InterfaceMeshCutUserObject that provides cut locations to this UserObject.");
  params.addRequiredParam<VariableName>(
      "level_set_var", "The name of level set variable used to represent the interface");
  params.addParam<bool>("debug",
                        false,
                        "Count the interface points that no rank (or this rank) evaluated on "
                        "each side. ExtendVelocityLevelSetAux prints the counts when its own "
                        "debug is true.");
  params.addParam<Real>("debug_sliver_fraction",
                        1e-2,
                        "With debug = true, every interface point whose side element has a "
                        "physical volume fraction below this gets a full per-node dump "
                        "([DBG-SLV]): node positions, physical flags, level set and variable "
                        "values and each node's contribution to the sampled gradient.");
  params.addParam<Real>("debug_gradient_threshold",
                        20.0,
                        "With debug = true, side elements that are not slivers but whose sampled "
                        "normal gradient is larger than this in magnitude are dumped too "
                        "([DBG-SLV] LARGEGRAD). Healthy values in the Pd/SiC decks are about 5 "
                        "or less.");
  params.addParam<unsigned int>("debug_reference_dumps",
                                2,
                                "With debug = true, number of well-resolved side elements "
                                "(volume fraction > 0.5) per execution dumped like the slivers "
                                "for comparison.");
  params.addClassDescription("Obtain field values and gradients on the interface.");
  return params;
}

QpPointValueAtXFEMInterface::QpPointValueAtXFEMInterface(const InputParameters & parameters)
  : GeneralUserObject(parameters),
    _mesh(_subproblem.mesh()),
    _var(&_subproblem.getVariable(_tid, parameters.get<VariableName>("variable"))),
    _var_level_set(&_subproblem.getVariable(_tid, parameters.get<VariableName>("level_set_var"))),
    _level_set_var_number(
        _subproblem.getVariable(_tid, parameters.get<VariableName>("level_set_var")).number()),
    _system(_subproblem.getSystem(getParam<VariableName>("level_set_var"))),
    _solution(*_system.current_local_solution.get()),
    _u_system(_subproblem.getSystem(getParam<VariableName>("variable"))),
    _u_solution(*_u_system.current_local_solution.get()),
    _debug(getParam<bool>("debug")),
    _dbg_sliver_fraction(getParam<Real>("debug_sliver_fraction")),
    _dbg_gradient_threshold(getParam<Real>("debug_gradient_threshold")),
    _dbg_n_reference(getParam<unsigned int>("debug_reference_dumps"))
{
  const VariableGradient & dummy = (dynamic_cast<MooseVariable *>(_var_level_set))->gradSln();
}

void
QpPointValueAtXFEMInterface::initialize()
{
  _pl = _mesh.getPointLocator();
  _xfem = MooseSharedNamespace::dynamic_pointer_cast<XFEM>(_fe_problem.getXFEM());
  if (_xfem == nullptr)
    mooseError("Problem casting to XFEM in QpPointValueAtXFEMInterface");

  const UserObject * uo =
      &(_fe_problem.getUserObjectBase(getParam<UserObjectName>("interface_mesh_cut_userobject")));

  if (dynamic_cast<const GeometricCutUserObject *>(uo) == nullptr)
    mooseError("UserObject casting to GeometricCutUserObject in QpPointValueAtXFEMInterface");

  _mesh_cut = dynamic_cast<const GeometricCutUserObject *>(uo);
  _elem_pairs = _xfem->getXFEMCutElemPairs(_xfem->getGeometricCutID(_mesh_cut));
}

void
QpPointValueAtXFEMInterface::execute()
{
  _values_positive_level_set_side.clear();
  _values_negative_level_set_side.clear();
  _grad_values_positive_level_set_side.clear();
  _grad_values_negative_level_set_side.clear();
  _level_set_normal.clear();
  _vf_positive_level_set_side.clear();
  _vf_negative_level_set_side.clear();
  _qp_points.clear();
  _dbg_slv_records.clear();
  _dbg_n_reference_done = 0;
  _dbg_seg_length.clear();
  _dbg_geom_normal.clear();

  std::vector<Point> qp_points_vector;
  for (std::list<std::pair<const Elem *, const Elem *>>::const_iterator it = _elem_pairs->begin();
       it != _elem_pairs->end();
       ++it)
  {
    const Elem * elem1 = it->first;

    std::vector<Point> intersectionPoints1;
    Point normal1;
    std::vector<Point> q_points1;
    std::vector<Real> weights1;

    unsigned int plane_id = 0; // Only support one cut plane for the time being
    _xfem->getXFEMIntersectionInfo(elem1, plane_id, normal1, intersectionPoints1, false);

    if (intersectionPoints1.size() == 2)
      _xfem->getXFEMqRuleOnLine(intersectionPoints1, q_points1, weights1);
    else if (intersectionPoints1.size() > 2)
      _xfem->getXFEMqRuleOnSurface(intersectionPoints1, q_points1, weights1);

    // Debug: segment length and geometric normal of the cut the points belong to
    if (_debug)
      for (const auto k : index_range(q_points1))
      {
        (void)k;
        _dbg_seg_length_vector.push_back(
            intersectionPoints1.size() == 2 ? (intersectionPoints1[0] - intersectionPoints1[1]).norm()
                                            : 0.0);
        _dbg_geom_normal_vector.push_back(normal1);
      }

    qp_points_vector.insert(qp_points_vector.end(), q_points1.begin(), q_points1.end());
  }

  for (unsigned int i = 0; i < qp_points_vector.size(); i++)
    _qp_points[i] = qp_points_vector[i];

  if (_debug)
  {
    for (const auto i : index_range(_dbg_seg_length_vector))
    {
      _dbg_seg_length[i] = _dbg_seg_length_vector[i];
      _dbg_geom_normal[i] = _dbg_geom_normal_vector[i];
    }
    _dbg_seg_length_vector.clear();
    _dbg_geom_normal_vector.clear();
  }

  _pl->enable_out_of_mesh_mode();

  unsigned int i = 0;
  for (const auto & pt : _qp_points)
  {
    const Elem * elem = getElemContainingPoint(pt.second, /*positive_level_set = */ true);

    if (elem != nullptr)
    {
      _subproblem.setCurrentSubdomainID(elem, /*_tid */ 0);
      _subproblem.reinitElemPhys(elem, {pt.second}, 0);

      _values_positive_level_set_side[i] = (dynamic_cast<MooseVariable *>(_var))->sln()[0];
      _grad_values_positive_level_set_side[i] =
          ((dynamic_cast<MooseVariable *>(_var))->gradSln())[0];

      // std::cout << "ptr = " << (dynamic_cast<MooseVariable *>(_var_level_set) == nullptr)
      //           << std::endl;

      //(dynamic_cast<MooseVariable *>(_var_level_set))->reinitAux();

      // std::cout << "var name = " << _var_level_set->name() << ", num = " << _level_set_var_number
      //           << std::endl;
      // std::cout << "u name = " << _var->name() << ", num = " << _var->number() << std::endl;

      // std::cout << "value = " << ((dynamic_cast<MooseVariable *>(_var_level_set))->sln())[0]
      //           << std::endl;
      // std::cout << "value2 = " << ((dynamic_cast<MooseVariable *>(_var_level_set))->gradSln())[0]
      //           << std::endl;

      _level_set_normal[i] = ((dynamic_cast<MooseVariable *>(_var_level_set))->gradSln())[0];
      _level_set_normal[i] /= _level_set_normal[i].norm();

      _vf_positive_level_set_side[i] = _xfem->getPhysicalVolumeFraction(elem);
      if (_debug)
        debugDumpElem(i, pt.second, *elem, true);
    }

    const Elem * elem2 = getElemContainingPoint(pt.second, false);
    if (elem2 != nullptr)
    {
      _subproblem.setCurrentSubdomainID(elem2, /*_tid */ 0);
      _subproblem.reinitElemPhys(elem2, {pt.second}, 0);

      _values_negative_level_set_side[i] = (dynamic_cast<MooseVariable *>(_var))->sln()[0];
      _grad_values_negative_level_set_side[i] =
          ((dynamic_cast<MooseVariable *>(_var))->gradSln())[0];

      _level_set_normal[i] = ((dynamic_cast<MooseVariable *>(_var_level_set))->gradSln())[0];
      _level_set_normal[i] /= _level_set_normal[i].norm();

      _vf_negative_level_set_side[i] = _xfem->getPhysicalVolumeFraction(elem2);
      if (_debug)
        debugDumpElem(i, pt.second, *elem2, false);
    }
    i++;
  }

  ++_dbg_exec_count;
  if (_debug)
  {
    _dbg_n_qp = _qp_points.size();
    _dbg_local_pos_missing = _dbg_n_qp - _values_positive_level_set_side.size();
    _dbg_local_neg_missing = _dbg_n_qp - _values_negative_level_set_side.size();
  }
}

void
QpPointValueAtXFEMInterface::finalize()
{
  _communicator.set_union(_values_positive_level_set_side);
  _communicator.set_union(_grad_values_positive_level_set_side);
  _communicator.set_union(_values_negative_level_set_side);
  _communicator.set_union(_grad_values_negative_level_set_side);
  _communicator.set_union(_level_set_normal);
  _communicator.set_union(_vf_positive_level_set_side);
  _communicator.set_union(_vf_negative_level_set_side);

  if (_debug)
  {
    _dbg_global_pos_missing = _dbg_n_qp - _values_positive_level_set_side.size();
    _dbg_global_neg_missing = _dbg_n_qp - _values_negative_level_set_side.size();
  }
}

void
QpPointValueAtXFEMInterface::debugDumpElem(const unsigned int i,
                                           const Point & p,
                                           const Elem & elem,
                                           const bool positive_side)
{
  // Called right after reinitElemPhys(elem, {p}), so sln(), gradSln() and gradPhi() are at p
  const Real vf = _xfem->getPhysicalVolumeFraction(&elem);
  auto * u_var = dynamic_cast<MooseVariable *>(_var);
  auto * ls_var = dynamic_cast<MooseVariable *>(_var_level_set);
  const RealGradient grad_u = u_var->gradSln()[0];
  const RealGradient grad_ls = ls_var->gradSln()[0];
  const Real grad_n = grad_u * (grad_ls / grad_ls.norm());

  // Large normal gradient on an element that is not a sliver (the spike is not only a sliver effect)
  const bool large_grad = std::abs(grad_n) > _dbg_gradient_threshold;
  const bool sliver = vf < _dbg_sliver_fraction;
  const bool reference = !sliver && !large_grad && vf > 0.5 && _dbg_n_reference_done < _dbg_n_reference;
  if (!sliver && !large_grad && !reference)
    return;
  if ((sliver || large_grad) && _dbg_slv_records.size() >= _dbg_max_slv_records)
    return;
  if (reference)
    ++_dbg_n_reference_done;

  const Real ls_at_p = ls_var->sln()[0];
  const Real u_at_p = u_var->sln()[0];
  const RealVectorValue n_ls = grad_ls / grad_ls.norm();
  const RealVectorValue n_geom = libmesh_map_find(_dbg_geom_normal, i);
  const Real seg_len = libmesh_map_find(_dbg_seg_length, i);
  const auto & grad_phi = u_var->gradPhi();

  // Nodes created by XFEM are the phantom nodes that stand in for the corners of the other side
  const std::set<unsigned int> new_nodes = _xfem->getNewNodes();
  unsigned int n_real = 0;
  Real contrib_real = 0.0, contrib_phantom = 0.0, delta_min = std::numeric_limits<Real>::max();
  std::ostringstream nodes;
  nodes << std::scientific << std::setprecision(5);
  for (const auto j : elem.node_index_range())
  {
    const Node & nd = elem.node_ref(j);
    const bool phantom = new_nodes.count(nd.id());
    const Real ls_j = _solution(nd.dof_number(_system.number(), _level_set_var_number, 0));
    const Real u_j = _u_solution(nd.dof_number(_u_system.number(), _var->number(), 0));
    const Real dist_n = (Point(nd) - p) * n_ls;
    const Real contrib = u_j * (grad_phi[j][0] * n_ls);
    (phantom ? contrib_phantom : contrib_real) += contrib;
    if (!phantom)
    {
      ++n_real;
      delta_min = std::min(delta_min, std::abs(dist_n));
    }
    nodes << "      node" << j << " id=" << nd.id() << " xy=(" << nd(0) << "," << nd(1)
          << ") phantom=" << phantom << " dist_to_interface_along_n=" << dist_n << " ls=" << ls_j
          << " u=" << u_j << " gradphi.n=" << (grad_phi[j][0] * n_ls) << " u*gradphi.n=" << contrib
          << "\n";
  }

  std::ostringstream o;
  o << std::scientific << std::setprecision(5) << "  [DBG-SLV] " << (sliver ? "SLIVER" : (large_grad ? "LARGEGRAD" : "REFERENCE"))
    << " qp=" << i << " side=" << (positive_side ? "pos" : "neg") << " elem=" << elem.id()
    << " vf=" << vf << " n_real_nodes=" << n_real << " seg_len=" << seg_len
    << " pt=(" << p(0) << "," << p(1) << ")\n"
    << "      ls_at_pt=" << ls_at_p << " (should be 0) u_at_pt=" << u_at_p
    << " grad_u=(" << grad_u(0) << "," << grad_u(1) << ") grad_u.n_ls=" << grad_u * n_ls
    << " grad_u_tangential=" << (grad_u - (grad_u * n_ls) * n_ls).norm()
    << " cos(n_ls,n_geom)=" << n_ls * n_geom << "\n"
    << "      contribution to grad_u.n from real nodes=" << contrib_real
    << " from phantom nodes=" << contrib_phantom
    << " nearest real node distance along n=" << delta_min << "\n"
    << nodes.str();
  _dbg_slv_records.push_back(o.str());
}

std::string
QpPointValueAtXFEMInterface::getDebugReport() const
{
  std::string report =
      "[rank " + std::to_string(processor_id()) + "] [DBG-QP] exec#" +
      std::to_string(_dbg_exec_count) + " n_qp=" + std::to_string(_dbg_n_qp) +
      " not_on_this_rank(pos,neg)=" + std::to_string(_dbg_local_pos_missing) + "," +
      std::to_string(_dbg_local_neg_missing) +
      " missing_on_all_ranks(pos,neg)=" + std::to_string(_dbg_global_pos_missing) + "," +
      std::to_string(_dbg_global_neg_missing) + "\n";
  for (const auto & r : _dbg_slv_records)
    report += r;
  return report;
}

const Elem *
QpPointValueAtXFEMInterface::getElemContainingPoint(const Point & p, bool positive_level_set)
{
  const Elem * elem1 = (*_pl)(p);

  if (elem1->processor_id() != processor_id())
    return nullptr;

  const Node * node = elem1->node_ptr(0);

  dof_id_type ls_dof_id = node->dof_number(_system.number(), _level_set_var_number, 0);

  Number ls_node_value = _solution(ls_dof_id);

  bool positive = false;

  if (_xfem->isPointInsidePhysicalDomain(elem1, *node))
  {
    // The interface is the zero level set, matching LevelSetCutUserObject
    if (ls_node_value > 0.0)
      positive = true;
  }
  else
  {
    if (ls_node_value < 0.0)
      positive = false;
  }

  const Elem * elem2 = nullptr;
  bool found = false;
  for (auto & pair : *_elem_pairs)
  {
    if (pair.first == elem1)
    {
      elem2 = pair.second;
      found = true;
    }
    else if (pair.second == elem1)
    {
      elem2 = pair.first;
      found = true;
    }
  }

  if (!found)
    mooseError("QpPointValueAtXFEMInterface: The interface node ",
               p,
               " are not found by element pair locator.");

  if ((positive && positive_level_set) || (!positive && !positive_level_set))
    return elem1;
  else if ((!positive && positive_level_set) || (positive && !positive_level_set))
    return elem2;
  else
    return nullptr;
}