//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "LevelSetNormalTimeDerivativeSUPG.h"

registerMooseObject("LevelSetApp", LevelSetNormalTimeDerivativeSUPG);

InputParameters
LevelSetNormalTimeDerivativeSUPG::validParams()
{
  InputParameters params = ADTimeKernelGrad::validParams();
  params.addClassDescription(
      "SUPG stablization terms for the time derivative of the level set equation.");
  params.addRequiredCoupledVar("velocity", "Velocity vector variable.");
  return params;
}

LevelSetNormalTimeDerivativeSUPG::LevelSetNormalTimeDerivativeSUPG(
    const InputParameters & parameters)
  : ADTimeKernelGrad(parameters), _velocity(adCoupledValue("velocity"))
{
}

ADRealVectorValue
LevelSetNormalTimeDerivativeSUPG::precomputeQpResidual()
{
  ADRealVectorValue velocity =
      _velocity[_qp] * _grad_u[_qp] / std::sqrt(_grad_u[_qp] * _grad_u[_qp] + 1.0e-50);

  ADReal tau = _current_elem->hmin() /
               (2 * (velocity + RealVectorValue(libMesh::TOLERANCE * libMesh::TOLERANCE)).norm());
  return tau * velocity * _u_dot[_qp];
}
