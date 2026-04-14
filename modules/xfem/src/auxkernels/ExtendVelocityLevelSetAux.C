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

  params.addRequiredParam<Real>("diffusivity_positive",
                                "Diffusion coefficient on the positive level set side");

  params.addRequiredParam<Real>("diffusivity_negative",
                                "Diffusion coefficient on the negative level set side");

  params.addRequiredParam<Real>("equilibrium_concentration_positive",
                                "Equilibrium concentration on the positive side of interface");

  params.addRequiredParam<Real>("equilibrium_concentration_negative",
                                "Equilibrium concentration on the negative side of interface");

  MooseEnum model_type("empirical_correlation INL_ROM Stefan", "empirical_correlation");
  params.addRequiredParam<MooseEnum>("model_type",
                                     model_type,
                                     "The type of model to apply. Options include: " +
                                         model_type.getRawNames());

  return params;
}

ExtendVelocityLevelSetAux::ExtendVelocityLevelSetAux(const InputParameters & parameters)
  : AuxKernel(parameters),
    _qp_value_uo(getUserObjectByName<QpPointValueAtXFEMInterface>(
        getParam<UserObjectName>("qp_point_value_user_object"))),

    _D_pos(getParam<Real>("diffusivity_positive")),
    _D_neg(getParam<Real>("diffusivity_negative")),

    _Ceq_pos(getParam<Real>("equilibrium_concentration_positive")),
    _Ceq_neg(getParam<Real>("equilibrium_concentration_negative")),

    _model_type(getParam<MooseEnum>("model_type").template getEnum<model_type>())
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

  Real vel = 0.0;
  switch (_model_type)
  {
    case model_type::empirical_correlation:
    {
      Real temperature_avg = 1000; // [K] SiC average temperature
      vel = 38.232 * std::exp(-11342.3 / temperature_avg);
      vel *= 1.0e-6 / (3600.0 * 24.0);
      break;
    }

    case model_type::INL_ROM:
    {
      Real V_m = 40.096 / 3.21 * 1.0e-6; // [m^3/mol] SiC molar volume
      Real net_flux = (-_D_pos * (_grad_values_positive_level_set_side[index] * _level_set_normal[index])) -
                      (-_D_neg * (_grad_values_negative_level_set_side[index] * _level_set_normal[index]));

      vel = 3 * V_m * net_flux;
      break;
    }

    case model_type::Stefan:
    {
      Real deltaC = _Ceq_pos - _Ceq_neg;

      if (std::abs(deltaC) < 1e-16)
        mooseError("Equilibrium concentrations are equal; interface velocity undefined.");

      Real net_flux = (-_D_pos * (_grad_values_positive_level_set_side[index] * _level_set_normal[index])) -
                      (-_D_neg * (_grad_values_negative_level_set_side[index] * _level_set_normal[index]));

      vel = net_flux / deltaC;
      break;
    }

    default:
      mooseError("Invalid model type.");
  }

  std::cout << "vel = " << vel << std::endl;
  return vel;
}