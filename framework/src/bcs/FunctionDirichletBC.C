//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "FunctionDirichletBC.h"
#include "Function.h"

registerMooseObject("MooseApp", FunctionDirichletBC);

InputParameters
FunctionDirichletBC::validParams()
{
  InputParameters params = DirichletBCBase::validParams();
  params.addRequiredParam<FunctionName>("function", "The forcing function.");
  params.addClassDescription(
      "Imposes the essential boundary condition $u=g(t,\\vec{x})$, where $g$ "
      "is a (possibly) time and space-dependent MOOSE Function.");
  return params;
}

FunctionDirichletBC::FunctionDirichletBC(const InputParameters & parameters)
  : DirichletBCBase(parameters),
    _func(getFunction("function")),
    _temp_bc({0.111607143, 1.97172619,  4.166666667, 7.068452381, 12.46279762,
              16.33184524, 20.20089286, 23.02827381, 26.07886905, 29.42708333,
              32.32886905, 34.30059524, 36.45833333, 38.4672619,  39.69494048,
              41.07142857, 42.187,      44.08482143, 46.39136905, 48.13988095},
             {840.15, 866.15, 893.15, 920.15, 973.15, 973.15, 973.15, 973.15, 973.15, 920.15,
              893.15, 866.15, 840.15, 786.15, 760.15, 733.15, 706.15, 680.15, 653.15, 626.15})

{
}

Real
FunctionDirichletBC::computeQpValue()
{
  Real y = (*_current_node)(1);
  return _temp_bc.sample(y);

  // return _func.value(_t, *_current_node);
}

// bool
// FunctionDirichletBC::shouldApply()
// {
//   Real x = (*_current_node)(0);
//   Real y = (*_current_node)(1);

//   if (std::sqrt(x * x + y * y) < 0.0025)
//     return true;
//   else
//     return false;
// }
