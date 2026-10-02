//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "QpPointValueAtXFEMInterface.h"
#include "GeneralPostprocessor.h"

/**
 *
 * Retrieves the position of a specified interface
 *
 */

class PositionOfXFEMInterfacePostprocessor : public GeneralPostprocessor
{
public:
  static InputParameters validParams();

  PositionOfXFEMInterfacePostprocessor(const InputParameters & parameters);

  virtual void initialize() override;

  virtual void execute() override {}

  virtual Real getValue() override;

protected:
  /// Pointer to PointValueAtXFEMInterface object
  const QpPointValueAtXFEMInterface * _value_at_interface_uo;
};
