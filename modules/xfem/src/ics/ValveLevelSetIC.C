//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "ValveLevelSetIC.h"
#include "Function.h"

registerMooseObject("MooseApp", ValveLevelSetIC);

InputParameters
ValveLevelSetIC::validParams()
{
  InputParameters params = InitialCondition::validParams();
  params.addRequiredParam<MeshFileName>("mesh_file", "Mesh file for the XFEM geometric cut.");

  params.addClassDescription("An initial condition that uses a normal function of x, y, z to "
                             "produce values (and optionally gradients) for a field variable.");
  params.addParam<Real>("scaling_factor", 1, "Scaling factor to apply on the function");

  return params;
}

ValveLevelSetIC::ValveLevelSetIC(const InputParameters & parameters) : InitialCondition(parameters)
{
  MeshFileName xfem_cutter_mesh_file = getParam<MeshFileName>("mesh_file");
  _cutter_mesh = std::make_shared<ReplicatedMesh>(_communicator);
  _cutter_mesh->read(xfem_cutter_mesh_file);
}

Real
ValveLevelSetIC::value(const Point & p)
{
  Real min_dist = std::numeric_limits<Real>::max();

  for (const auto & cut_elem : _cutter_mesh->element_ptr_range())
  {
    Point a = cut_elem->node_ref(0);
    Point b = cut_elem->node_ref(1);

    Point c = p - a;
    Point v = (b - a) / (b - a).norm();
    Real d = (b - a).norm();
    Real t = v * c;

    Real dist;
    Point nearest_point;

    if (t < 0)
    {
      dist = (p - a).norm();
      nearest_point = a;
    }
    else if (t > d)
    {
      dist = (p - b).norm();
      nearest_point = b;
    }
    else
    {
      v *= t;
      dist = (p - a - v).norm();
      nearest_point = (a + v);
    }

    Point p_nearest_point = nearest_point - p;

    Point normal_ab = Point(-(b - a)(1), (b - a)(0), 0);

    if (normal_ab * p_nearest_point < 0)
      dist = -dist;

    if (std::abs(dist) < std::abs(min_dist))
      min_dist = dist;
  }

  return min_dist;
}

RealGradient
ValveLevelSetIC::gradient(const Point & p)
{
  return 0.0;
}
