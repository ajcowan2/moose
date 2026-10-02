//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

// MOOSE includes
#include "LevelSetOlssonBubbles.h"

registerMooseObject("LevelSetApp", LevelSetOlssonBubbles);

InputParameters
LevelSetOlssonBubbles::validParams()
{
  InputParameters params = Function::validParams();
  params.addClassDescription("Implementation of 'bubble' ranging from 0 to 1.");
  params.addParam<std::vector<Point>>("centers", "The center of the bubble.");
  params.addParam<std::vector<Real>>("radii", "The radius of the bubble.");
  params.addParam<Real>("epsilon", 0.01, "The interface thickness.");
  return params;
}

LevelSetOlssonBubbles::LevelSetOlssonBubbles(const InputParameters & parameters)
  : Function(parameters),
    _centers(getParam<std::vector<Point>>("centers")),
    _radii(getParam<std::vector<Real>>("radii")),
    _epsilon(getParam<Real>("epsilon"))
{
  if (_centers.size() != _radii.size())
    mooseError("In LevelSetOlssonBubbles, centers and radii's size must be equal.");
}

Real
LevelSetOlssonBubbles::value(Real /*t*/, const Point & p) const
{
  Real min = std::numeric_limits<Real>::max();

  for (unsigned int i = 0; i < _centers.size(); ++i)
  {
    const auto x = ((p - _centers[i]).norm() - _radii[i]) / _epsilon;
    if (x < min)
      min = x;
  }
  return 1.0 / (1 + std::exp(min));
}

ADReal
LevelSetOlssonBubbles::value(const ADReal & /*t*/, const ADPoint & p) const
{
  ADReal min = std::numeric_limits<Real>::max();

  for (unsigned int i = 0; i < _centers.size(); ++i)
  {
    const auto x = ((p - _centers[i]).norm() - _radii[i]) / _epsilon;
    if (x < min)
      min = x;
  }
  return 1.0 / (1 + std::exp(min));
}

RealGradient
LevelSetOlssonBubbles::gradient(Real /*t*/, const Point & p) const
{
  return RealGradient(0.0);
}
