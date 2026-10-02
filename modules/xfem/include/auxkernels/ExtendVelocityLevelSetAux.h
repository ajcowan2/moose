//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "AuxKernel.h"
#include "QpPointValueAtXFEMInterface.h"

class QpPointValueAtXFEMInterface;

/**
 * Extends velocity from an interface to a domain
 */
class ExtendVelocityLevelSetAux : public AuxKernel
{
public:
  static InputParameters validParams();

  ExtendVelocityLevelSetAux(const InputParameters & parameters);

protected:
  virtual Real computeValue() override;

  /// Pointer to the QpPointValueAtXFEMInterface object
  const QpPointValueAtXFEMInterface & _qp_value_uo;

  /// Mapping from point index and its values at the positive level set side
  std::map<unsigned int, Real> _values_positive_level_set_side;

  /// Mapping from point index and its values at the negative level set side
  std::map<unsigned int, Real> _values_negative_level_set_side;

  /// Mapping from point index and its gradient at the positive level set side
  std::map<unsigned int, RealVectorValue> _grad_values_positive_level_set_side;

  /// Mapping from point index and its gradient at the negative level set side
  std::map<unsigned int, RealVectorValue> _grad_values_negative_level_set_side;

  /// Mapping from point index and its gradient at the negative level set side
  std::map<unsigned int, RealVectorValue> _level_set_normal;

  std::map<unsigned int, Point> _qp_points;
};
