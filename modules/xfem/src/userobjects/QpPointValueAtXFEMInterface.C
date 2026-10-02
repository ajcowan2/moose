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
    _solution(*_system.current_local_solution.get())
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
  _qp_points.clear();

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

    qp_points_vector.insert(qp_points_vector.end(), q_points1.begin(), q_points1.end());
  }

  for (unsigned int i = 0; i < qp_points_vector.size(); i++)
    _qp_points[i] = qp_points_vector[i];

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
    }
    i++;
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
    if (ls_node_value > 0.5)
      positive = true;
  }
  else
  {
    if (ls_node_value < 0.5)
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
