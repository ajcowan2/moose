//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "InitialCondition.h"

#include "libmesh/exodusII_io.h"
#include "libmesh/explicit_system.h"
#include "libmesh/equation_systems.h"
#include "Function.h"
#include "libmesh/enum_to_string.h"
#include "XFEMFuncs.h"
#include "libmesh/exodusII_io.h"
#include "libmesh/explicit_system.h"
#include "libmesh/equation_systems.h"

#include <string>

class Function;
class InputParameters;

template <typename T>
InputParameters validParams();

/**
 * Defines a boundary condition that forces the value to be a user specified
 * function at the boundary.
 */
class ValveLevelSetIC : public InitialCondition
{
public:
  static InputParameters validParams();

  ValveLevelSetIC(const InputParameters & parameters);

protected:
  /**
   * Evaluate the function at the current quadrature point and time step.
   */
  Real f();

  /**
   * The value of the variable at a point.
   */
  virtual Real value(const Point & p) override;

  /**
   * The value of the gradient at a point.
   */
  virtual RealGradient gradient(const Point & p) override;

  /// The cutter mesh
  std::shared_ptr<MeshBase> _cutter_mesh;
};
