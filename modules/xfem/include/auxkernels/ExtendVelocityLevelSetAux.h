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

#include <set>
#include <vector>

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

  /// Diffusion coefficients
  const PostprocessorValue & _D_pos;
  const PostprocessorValue & _D_neg;

  /// Scalar multiplier applied to the computed velocity
  Real _scale_factor;

  /// Grain-boundary volume fraction g of the converted material (STEFAN): jump_c -> g * jump_c
  const Real _gb_volume_fraction;

  /// STEFAN only: volume swept per mole of Pd consumed [m^3/mol]; 0 = Pd2Si product volume V_Pd2Si / 2 (1 / jump_c)
  const Real _stefan_volume_per_Pd;

  /// The model type to use for velocity calculation
  const enum class model_type { empirical_correlation, INL_ROM, Stefan, pit_corrosion } _model_type;

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

  /// Print per-interface-point velocity terms (see debug parameter)
  const bool _debug;

  /// Debug: only print at time steps that are a multiple of this number
  const unsigned int _debug_interval;

  /// Debug: value of the user object execute counter at the last printed sweep
  unsigned int _dbg_last_exec_count = 0;

  /// Debug: interface point indices already printed in the current sweep
  std::set<unsigned int> _dbg_printed_qps;
};