//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "LevelSetNormalAdvectionSUPG.h"

registerMooseObject("LevelSetApp", LevelSetNormalAdvectionSUPG);

InputParameters
LevelSetNormalAdvectionSUPG::validParams()
{
  InputParameters params = ADKernelGrad::validParams();
  params.addClassDescription(
      "SUPG stablization term for the advection portion of the level set equation.");
  params.addRequiredCoupledVar("velocity", "Velocity vector variable.");
  return params;
}

LevelSetNormalAdvectionSUPG::LevelSetNormalAdvectionSUPG(const InputParameters & parameters)
  : ADKernelGrad(parameters), _velocity(adCoupledValue("velocity"))
{
}

ADRealVectorValue
LevelSetNormalAdvectionSUPG::precomputeQpResidual()
{
  ADRealVectorValue velocity =
      _velocity[_qp] * _grad_u[_qp] / std::sqrt(_grad_u[_qp] * _grad_u[_qp] + 1.0e-50);

  ADReal tau = _current_elem->hmin() /
               (2 * (velocity + RealVectorValue(libMesh::TOLERANCE * libMesh::TOLERANCE)).norm());
  return (tau * velocity) * (velocity * _grad_u[_qp]);
}
