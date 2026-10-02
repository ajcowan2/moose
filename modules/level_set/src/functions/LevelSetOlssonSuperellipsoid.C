//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

// MOOSE includes
#include "LevelSetOlssonSuperellipsoid.h"

registerMooseObject("LevelSetApp", LevelSetOlssonSuperellipsoid);

InputParameters
LevelSetOlssonSuperellipsoid::validParams()
{
  InputParameters params = Function::validParams();
  params.addClassDescription("Implementation of 'bubble' ranging from 0 to 1.");
  params.addParam<Point>("center", "The center of the bubble.");
  params.addParam<Real>("a", "The radius of the bubble.");
  params.addParam<Real>("b", "The radius of the bubble.");
  params.addParam<Real>("c", "The radius of the bubble.");
  params.addParam<Real>("n", "The radius of the bubble.");
  params.addParam<Real>("epsilon", 0.01, "The interface thickness.");
  return params;
}

LevelSetOlssonSuperellipsoid::LevelSetOlssonSuperellipsoid(const InputParameters & parameters)
  : Function(parameters),
    _center(getParam<Point>("center")),
    _a(getParam<Real>("a")),
    _b(getParam<Real>("b")),
    _c(getParam<Real>("c")),
    _n(getParam<Real>("n")),
    _epsilon(getParam<Real>("epsilon"))
{
}

Real
LevelSetOlssonSuperellipsoid::value(Real /*t*/, const Point & p) const
{
  Real dist = (_center - p).norm();
  Point dist_vec = p - _center;

  Real rmn = (std::pow(std::abs(dist_vec(0) / dist / _a), _n) +
              std::pow(std::abs(dist_vec(1) / dist / _b), _n) +
              std::pow(std::abs(dist_vec(2) / dist / _c), _n));
  // Then calculate r from rmn
  Real r = std::pow(rmn, (-1.0 / _n));

  return 1.0 / (1 + std::exp((dist - r) / _epsilon));
}

ADReal
LevelSetOlssonSuperellipsoid::value(const ADReal & /*t*/, const ADPoint & p) const
{
  ADReal dist = (_center - p).norm();
  ADPoint dist_vec = p - _center;

  ADReal rmn = (std::pow(std::abs(dist_vec(0) / dist / _a), _n) +
                std::pow(std::abs(dist_vec(1) / dist / _b), _n) +
                std::pow(std::abs(dist_vec(2) / dist / _c), _n));
  // Then calculate r from rmn
  ADReal r = std::pow(rmn, (-1.0 / _n));

  return 1.0 / (1 + std::exp((dist - r) / _epsilon));
}

RealGradient
LevelSetOlssonSuperellipsoid::gradient(Real /*t*/, const Point & p) const
{
  return RealGradient(0.0);
}
