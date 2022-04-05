//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

// MOOSE includes
#include "LevelSetOlssonTwoBubbles.h"

registerMooseObject("LevelSetApp", LevelSetOlssonTwoBubbles);

InputParameters
LevelSetOlssonTwoBubbles::validParams()
{
  InputParameters params = Function::validParams();
  params.addClassDescription("Implementation of 'bubble' ranging from 0 to 1.");
  params.addParam<RealVectorValue>(
      "center1", RealVectorValue(0.5, 0.5, 0), "The center of the bubble.");
  params.addParam<Real>("radius1", 0.15, "The radius of the bubble.");
  params.addParam<RealVectorValue>(
      "center2", RealVectorValue(0.5, 0.5, 0), "The center of the bubble.");
  params.addParam<Real>("radius2", 0.15, "The radius of the bubble.");
  params.addParam<Real>("epsilon", 0.01, "The interface thickness.");
  return params;
}

LevelSetOlssonTwoBubbles::LevelSetOlssonTwoBubbles(const InputParameters & parameters)
  : Function(parameters),
    _center1(getParam<RealVectorValue>("center1")),
    _radius1(getParam<Real>("radius1")),
    _center2(getParam<RealVectorValue>("center2")),
    _radius2(getParam<Real>("radius2")),
    _epsilon(getParam<Real>("epsilon"))
{
}

Real
LevelSetOlssonTwoBubbles::value(Real /*t*/, const Point & p) const
{
  const auto x1 = ((p - _center1).norm() - _radius1) / _epsilon;
  const auto x2 = ((p - _center2).norm() - _radius2) / _epsilon;
  const auto x = std::min(x1, x2);
  return 1.0 / (1 + std::exp(x));
}

ADReal
LevelSetOlssonTwoBubbles::value(const ADReal & /*t*/, const ADPoint & p) const
{
  const auto x1 = ((p - _center1).norm() - _radius1) / _epsilon;
  const auto x2 = ((p - _center2).norm() - _radius2) / _epsilon;
  const auto x = std::min(x1, x2);
  return 1.0 / (1 + std::exp(x));
}

RealGradient
LevelSetOlssonTwoBubbles::gradient(Real /*t*/, const Point & p) const
{
  Real norm1 = (p - _center1).norm();
  Real g1 = (norm1 - _radius1) / _epsilon;
  RealGradient output1;

  Real g_prime1;
  for (unsigned int i = 0; i < LIBMESH_DIM; ++i)
  {
    g_prime1 = (p(i) - _center1(i)) / (_epsilon * norm1);
    output1(i) = -(g_prime1 * std::exp(g1)) / ((std::exp(g1) + 1) * (std::exp(g1) + 1));
  }

  Real norm2 = (p - _center2).norm();
  Real g2 = (norm1 - _radius2) / _epsilon;
  RealGradient output2;

  Real g_prime2;
  for (unsigned int i = 0; i < LIBMESH_DIM; ++i)
  {
    g_prime2 = (p(i) - _center2(i)) / (_epsilon * norm2);
    output2(i) = -(g_prime2 * std::exp(g2)) / ((std::exp(g2) + 1) * (std::exp(g2) + 1));
  }

  const auto x1 = ((p - _center1).norm() - _radius1) / _epsilon;
  const auto x2 = ((p - _center2).norm() - _radius2) / _epsilon;

  if (x1 < x2)
    return output1;
  else
    return output2;
}
