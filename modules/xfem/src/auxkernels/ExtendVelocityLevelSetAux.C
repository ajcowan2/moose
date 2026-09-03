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

  params.addRequiredParam<PostprocessorName>("diffusivity_positive",
                                             "Postprocessor supplying the positive-side diffusivity");

  params.addRequiredParam<PostprocessorName>("diffusivity_negative",
                                             "Postprocessor supplying the negative-side diffusivity");

  params.addParam<Real>("scale_factor",
                         1.0,
                         "Scalar multiplier applied to the computed velocity");

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

    _D_pos(getPostprocessorValue("diffusivity_positive")),
    _D_neg(getPostprocessorValue("diffusivity_negative")),

    _scale_factor(getParam<Real>("scale_factor")),
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

  unsigned index1 = 0;
  unsigned index2 = 0;
  Real min_dist1 = std::numeric_limits<Real>::max();
  Real min_dist2 = std::numeric_limits<Real>::max();
  for (auto const & qp : _qp_points)
  {
    Real dist = (*_current_node - qp.second).norm();
    if (dist < min_dist1)
    {
      min_dist2 = min_dist1;
      index2 = index1;

      min_dist1 = dist;
      index1 = qp.first;
    }
    else if (dist < min_dist2)
    {
      min_dist2 = dist;
      index2 = qp.first;
    }
  }

  Real w1, w2;
  if (min_dist1 < std::numeric_limits<Real>::epsilon())
  {
    w1 = 1.0; w2 = 0.0;
  }
  else if (min_dist2 == std::numeric_limits<Real>::max())
  {
    w1 = 1.0; w2 = 0.0;
  }
  else
  {
    Real inv1 = 1.0 / min_dist1;
    Real inv2 = 1.0 / min_dist2;
    w1 = inv1 / (inv1 + inv2);
    w2 = inv2 / (inv1 + inv2);
  }

  Real vel = 0.0;
  switch (_model_type)
  {
    case model_type::empirical_correlation:
    {
      Real temperature_avg = 1000; // [K] SiC average temperature
      vel = 38.232 * std::exp(-11342.3 / temperature_avg);
      vel *= 1.0e-6 / (3600.0 * 24.0);
      vel *= _scale_factor;
      break;
    }

    case model_type::INL_ROM:
    {
      Real V_m = 40.096 / 3.21 * 1.0e-6; // [m^3/mol] SiC molar volume

      Real flux_pos1 = -_D_pos * (_grad_values_positive_level_set_side[index1] * _level_set_normal[index1]);
      Real flux_neg1 = -_D_neg * (_grad_values_negative_level_set_side[index1] * _level_set_normal[index1]);
      Real vel1 = 3 * V_m * (flux_pos1 + flux_neg1);

      Real flux_pos2 = -_D_pos * (_grad_values_positive_level_set_side[index2] * _level_set_normal[index2]);
      Real flux_neg2 = -_D_neg * (_grad_values_negative_level_set_side[index2] * _level_set_normal[index2]);
      Real vel2 = 3 * V_m * (flux_pos2 + flux_neg2);

      vel = w1 * vel1 + w2 * vel2;
      vel *= _scale_factor;
      break;
    }

    case model_type::Stefan:
    {
      Real V_m_fu   = (2*106.42 + 28.085) / 9.59 * 1.0e-6;  // Pd2Si molar volume [m^3/mol]
      Real n_Pd_fu  = 2.0;                                   // Pd atoms per Pd2Si
      Real jump_c   = n_Pd_fu / V_m_fu;                      // concentration jump [mol/m^3]

      Real flux_pos1 = -_D_pos * (_grad_values_positive_level_set_side[index1] * _level_set_normal[index1]);
      Real flux_neg1 = -_D_neg * (_grad_values_negative_level_set_side[index1] * _level_set_normal[index1]);
      // Real vel1 = V_m / mol_frac * (flux_pos1 + flux_neg1);
      Real vel1 = -flux_neg1 / jump_c;

      Real flux_pos2 = -_D_pos * (_grad_values_positive_level_set_side[index2] * _level_set_normal[index2]);
      Real flux_neg2 = -_D_neg * (_grad_values_negative_level_set_side[index2] * _level_set_normal[index2]);
      // Real vel2 = V_m / mol_frac * (flux_pos2 + flux_neg2);
      Real vel2 = -flux_neg2 / jump_c;

      vel = w1 * vel1 + w2 * vel2;
      vel *= _scale_factor;
      break;
    }

    default:
      mooseError("Invalid model type.");
  }

  // std::cout << "vel = " << vel << std::endl;
  return vel;
}