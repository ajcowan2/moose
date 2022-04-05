//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "ExtendVelocityLevelSetAux.h"
#include "InterfaceMeshCutUserObjectBase.h"

registerMooseObject("XFEMApp", ExtendVelocityLevelSetAux);

InputParameters
ExtendVelocityLevelSetAux::validParams()
{
  InputParameters params = AuxKernel::validParams();
  params.addClassDescription("Extends velocity from an interface to a domain.");
  params.addParam<UserObjectName>(
      "qp_point_value_user_object",
      "Name of QpPointValueAtXFEMInterface that gives values at Qp points along an interface.");
  return params;
}

ExtendVelocityLevelSetAux::ExtendVelocityLevelSetAux(const InputParameters & parameters)
  : AuxKernel(parameters),
    _qp_value_uo(getUserObjectByName<QpPointValueAtXFEMInterface>(
        getParam<UserObjectName>("qp_point_value_user_object")))
{
  if (!isNodal())
    mooseError("ExtendVelocityLevelSetAux: Aux variable must be nodal variable.");
}

Real
ExtendVelocityLevelSetAux::computeValue()
{
  _values_positive_level_set_side = _qp_value_uo.getValueAtPositiveLevelSet();
  _values_negative_level_set_side = _qp_value_uo.getValueAtNegativeLevelSet();
  _grad_values_positive_level_set_side = _qp_value_uo.getGradientAtPositiveLevelSet();
  _grad_values_negative_level_set_side = _qp_value_uo.getGradientAtNegativeLevelSet();
  _level_set_normal = _qp_value_uo.getLevelSetNormal();

  // for (auto const & v : _grad_values_negative_level_set_side)
  //   std::cout << "_grad_values_negative_level_set_side = " << v.second << std::endl;

  // for (unsigned int i = 0; i < _grad_values_positive_level_set_side.size(); i++)
  //   std::cout << "qp = " << _qp_points[i] << "term a = " <<
  //   _grad_values_positive_level_set_side[i]
  //             << ", term b = " << _level_set_normal[i]
  //             << "dot product = " << _grad_values_positive_level_set_side[i] *
  //             _level_set_normal[i]
  //             << " value = " << _values_negative_level_set_side[i] << std::endl;

  _qp_points = _qp_value_uo.getQpPoint();

  unsigned index = 0;
  Real min_dist = std::numeric_limits<Real>::max();
  for (auto const & qp : _qp_points)
  {
    Real dist = (*_current_node - qp.second).norm();
    if (dist < min_dist)
    {
      min_dist = dist;
      index = qp.first;
    }
  }

  // std::cout << "grad u = " << _grad_values_negative_level_set_side[index] << std::endl;
  // std::cout << "pos u = " << _values_positive_level_set_side[index] << std::endl;
  // std::cout << "neg u = " << _values_negative_level_set_side[index] << std::endl;

  // Real vel = -0.796e-5 * _grad_values_negative_level_set_side[index] * RealVectorValue(0, -1, 0)
  // /
  //            (_values_negative_level_set_side[index] - _values_positive_level_set_side[index]);

  // Real vel = 0.8102e-5 * _grad_values_positive_level_set_side[index] * _level_set_normal[index] /
  //            (_values_positive_level_set_side[index] - 143.0);

  Real vel = 0.8102e-5 * _grad_values_positive_level_set_side[index] * _level_set_normal[index] /
             (_values_positive_level_set_side[index] - 143.0);

  // std::cout << "grad = " << _grad_values_positive_level_set_side[index] << std::endl;
  // std::cout << "normal = " << _level_set_normal[index] << std::endl;
  // std::cout << "post = " << _values_positive_level_set_side[index] << std::endl;

  // return std::abs(vel);

  return -0.0113 / (1 + exp(-0.1 * (_values_positive_level_set_side[index] - 920)));
}
