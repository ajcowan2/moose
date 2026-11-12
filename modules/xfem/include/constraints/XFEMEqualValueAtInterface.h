//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "ElemElemConstraint.h"

#include <map>

// Forward Declarations
class XFEM;

class XFEMEqualValueAtInterface : public ElemElemConstraint
{
public:
  static InputParameters validParams();

  XFEMEqualValueAtInterface(const InputParameters & parameters);
  virtual ~XFEMEqualValueAtInterface();

protected:
  virtual void reinitConstraintQuadrature(const ElementPairInfo & element_pair_info) override;

  virtual Real computeQpResidual(Moose::DGResidualType type) override;

  virtual Real computeQpJacobian(Moose::DGJacobianType type) override;

  /**
   * Count the level set nodes of the neighbors of the current element that lie on each side of
   * the interface (nodes created by XFEM are skipped).
   * @param count_pos Number of neighbor nodes with a non-negative level set value
   * @param count_neg Number of neighbor nodes with a negative level set value
   * @return True if the current element is on the positive level set side
   */
  bool elemOnPositiveSide(unsigned int & count_pos, unsigned int & count_neg) const;

  /**
   * Scale factor C of the penalty term alpha * (u - g) * test * C.
   * @param D Diffusivity on the side of the element
   * @param area Cut plane area
   * @param phys_vol Physical volume of the element
   * @return sqrt(|D area / vol|) or, with penalty_scaling = linear, |D area / vol|
   */
  Real penaltyFactor(const Real D, const Real area, const Real phys_vol) const;

  /// Vector normal to the internal interface
  Point _interface_normal;

  // Penalty parameter in penalty formulation
  Real _alpha;

  /// Value at the interface
  Real _value;

  Real _value_neighbor;

  /// Pointer to the XFEM controller object
  std::shared_ptr<XFEM> _xfem;

  /// The variable number of the level set variable we are operating on
  const unsigned int _level_set_var_number;

  /// system reference
  const System & _system;

  /// the subproblem solution vector
  const NumericVector<Number> & _solution;

  const bool _use_penalty;

  const Real & _diff;

  /// Optional diffusivity on the positive level set side; with _diff_negative it replaces _diff
  const PostprocessorValue * const _diff_positive;

  /// Optional diffusivity on the negative level set side
  const PostprocessorValue * const _diff_negative;

  /// True if the penalty scale factor is linear in D * area / vol (dimensionally consistent)
  const bool _linear_penalty;

  /// Multiply the interface quadrature weights by the coordinate transformation factor (2 pi r in RZ)
  const bool _rz_interface_weighting;

  /// Print per-element-pair diagnostics (see debug parameter)
  const bool _debug;

  /// Debug: only write the records of time steps that are a multiple of this number
  const unsigned int _debug_interval;

  /// Latest residual evaluation of a cut element pair, kept for the debug output
  struct DebugRecord
  {
    int step = 0;
    Real t = 0;
    dof_id_type neighbor = 0;
    Real cx = 0, cy = 0, vf_elem = 0, vf_neigh = 0, area = 0;
    unsigned int count_pos = 0, count_neg = 0;
    bool use_positive = false;
    Real u_elem = 0, u_neigh = 0, mismatch_elem = 0, mismatch_neigh = 0;
    Real gradn_elem = 0, gradn_neigh = 0, D_elem = 0, D_neigh = 0, C_elem = 0, C_neigh = 0;
    Real flux_term = 0, adjoint_term = 0, penalty_term = 0;
  };

  /// Debug: latest record per element id, written when the time step changes
  std::map<dof_id_type, DebugRecord> _dbg_last;

  /// Debug: time step of the records in _dbg_last
  int _dbg_step = -1;

  /// Write the records of _dbg_last to the per-rank debug file and clear them
  void debugFlush();

  /// Debug: only print element pairs whose current element centroid is within this distance of
  /// _debug_point (a negative value disables the filter)
  const Real _debug_radius;

  /// Debug: center of the printing window
  const Point _debug_point;
};
