//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "LevelSetNormalAdvection.h"

registerMooseObject("LevelSetApp", LevelSetNormalAdvection);

InputParameters
LevelSetNormalAdvection::validParams()
{
  InputParameters params = ADKernelValue::validParams();
  params.addClassDescription("Implements the level set advection equation: $\\vec{v}\\cdot\\nabla "
                             "u = 0$, where the weak form is $(\\psi_i, \\vec{v}\\cdot\\nabla u) = "
                             "0$.");
  params.addRequiredCoupledVar("velocity", "Velocity vector variable.");
  return params;
}

LevelSetNormalAdvection::LevelSetNormalAdvection(const InputParameters & parameters)
  : ADKernelValue(parameters), _velocity(adCoupledValue("velocity"))
{
}

ADReal
LevelSetNormalAdvection::precomputeQpResidual()
{
  return -_velocity[_qp] * std::sqrt(_grad_u[_qp] * _grad_u[_qp] + 1.0e-50);
}
