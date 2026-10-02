//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "ElemElemConstraint.h"
#include "LinearInterpolation.h"

// Forward Declarations
class XFEM;

class XFEMDirichletBC : public ElemElemConstraint
{
public:
  static InputParameters validParams();

  XFEMDirichletBC(const InputParameters & parameters);
  virtual ~XFEMDirichletBC();

protected:
  virtual void reinitConstraintQuadrature(const ElementPairInfo & element_pair_info) override;

  virtual Real computeQpResidual(Moose::DGResidualType type) override;

  virtual Real computeQpJacobian(Moose::DGJacobianType type) override;

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

  const LinearInterpolation _temp_bc;
};
