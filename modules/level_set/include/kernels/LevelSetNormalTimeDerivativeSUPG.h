//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "ADTimeKernelGrad.h"

/**
 * Applies SUPG stabilization to the time derivative.
 */
class LevelSetNormalTimeDerivativeSUPG : public ADTimeKernelGrad
{
public:
  static InputParameters validParams();

  LevelSetNormalTimeDerivativeSUPG(const InputParameters & parameters);

protected:
  virtual ADRealVectorValue precomputeQpResidual() override;

  /// Velocity vector variable
  const ADVariableValue & _velocity;
};
