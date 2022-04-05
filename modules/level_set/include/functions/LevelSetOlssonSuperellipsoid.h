//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

// MOOSE includes
#include "Function.h"

/**
 * Implements the "bubble" function from Olsson and Kreiss (2005).
 */
class LevelSetOlssonSuperellipsoid : public Function
{
public:
  static InputParameters validParams();

  LevelSetOlssonSuperellipsoid(const InputParameters & parameters);

  using Function::value;
  virtual Real value(Real /*t*/, const Point & p) const override;
  virtual ADReal value(const ADReal & /*t*/, const ADPoint & p) const override;

  virtual RealGradient gradient(Real /*t*/, const Point & p) const override;

protected:
  /// The 'center' of the bubble
  const Point & _center;

  /// The radius of the bubble
  const Real & _a;
  const Real & _b;
  const Real & _c;
  const Real & _n;

  const Real & _epsilon;
};
