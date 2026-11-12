//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "GeneralUserObject.h"

#include "libmesh/system.h"

/**
 * Reinitializes a level set variable as the exact signed distance to its own zero contour.
 *
 * The zero contour is the piecewise linear curve that LevelSetCutUserObject cuts along: on every
 * element edge with ls1 * ls2 < 0 it crosses at |ls1| / |ls1 - ls2| from node 1. Every node is
 * then given the sign of its old value times its Euclidean distance to that curve. Because the
 * contour is rebuilt from the same edge crossings, it is not moved (up to curvature within an
 * element), no pseudo-time equation is solved, and |grad ls| = 1 everywhere afterwards, which also
 * removes any node-to-node oscillation in the far field. The level set must be a first order
 * nodal variable on a 2D mesh.
 */
class LevelSetGeometricReinitialization : public GeneralUserObject
{
public:
  static InputParameters validParams();

  LevelSetGeometricReinitialization(const InputParameters & parameters);

  virtual void initialize() override {}
  virtual void execute() override;
  virtual void finalize() override {}

protected:
  /// Level set system
  libMesh::System & _system;

  /// Variable number of the level set in its system
  const unsigned int _var_number;

  /// Print a summary per reinitialization and per-rank details (see debug parameter)
  const bool _debug;

  /// Debug: only print at time steps that are a multiple of this number
  const unsigned int _debug_interval;
};
