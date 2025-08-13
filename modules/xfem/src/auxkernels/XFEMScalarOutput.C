//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "XFEMScalarOutput.h"
#include "petscblaslapack.h"
#include "XFEM.h"

registerMooseObject("XFEMApp", XFEMScalarOutput);

InputParameters
XFEMScalarOutput::validParams()
{
  InputParameters params = AuxKernel::validParams();
  params.addClassDescription(
      "Computes the volume fraction of the physical material in each partial element.");
  params.addRequiredParam<std::string>("property_name", "The name of the scalar material property to project");
  return params;
}

XFEMScalarOutput::XFEMScalarOutput(const InputParameters & parameters)
  : AuxKernel(parameters),
    _scalar_property(getMaterialProperty<Real>(parameters.get<std::string>("property_name")))
{
  if (isNodal())
    mooseError("XFEMScalarOutput must be run on an element variable");

  FEProblemBase * fe_problem = dynamic_cast<FEProblemBase *>(&_subproblem);
  if (fe_problem == nullptr)
    mooseError("Problem casting _subproblem to FEProblemBase in XFEMScalarOutput");

  _xfem = MooseSharedNamespace::dynamic_pointer_cast<XFEM>(fe_problem->getXFEM());
  if (_xfem == nullptr)
    mooseError("Problem casting to XFEM in XFEMScalarOutput");
}

Real
XFEMScalarOutput::computeValue()
{
  // std::cout << "elem id = " << _current_elem->id() << std::endl;
  // std::cout << _xfem->getPhyiscalCenterPoint(_current_elem) << std::endl;
  return _xfem->getPhysicalVolumeFraction(_current_elem);
}

void
XFEMScalarOutput::compute()
{
  precalculateValue();
  if (_xfem->isElemCut(_current_elem))
  {
    Point cp = _xfem->getPhyiscalCenterPoint(_current_elem);

    // std::cout << _current_elem->id() << std::endl;
    // std::cout << "center point = " << cp << std::endl;

    unsigned int N = _qrule->n_points();

    std::vector<Real> known_values;
    known_values.resize(N);
    for (unsigned int i = 0; i < N; ++i)
    {
      known_values[i] = _scalar_property[i];
    }

    // Build A matrix
    std::vector<std::vector<Real>> A(N, std::vector<double>(4, 0.0));
    for (unsigned int i = 0; i < N; ++i)
    {
      Real x = _q_point[i](0);
      Real y = _q_point[i](1);
      A[i][0] = 1.0;
      A[i][1] = x;
      A[i][2] = y;
      A[i][3] = x * y;
    }

    // Solve least squares: (A^T * A) * coeffs = A^T * b
    std::vector<std::vector<double>> AtA(4, std::vector<double>(4, 0.0));
    std::vector<double> Atb(4, 0.0);

    for (unsigned int i = 0; i < N; ++i)
    {
      for (int m = 0; m < 4; ++m)
      {
        Atb[m] += A[i][m] * known_values[i];
        for (int n = 0; n < 4; ++n)
        {
          AtA[m][n] += A[i][m] * A[i][n];
        }
      }
    }

    // Solve system AtA * coeffs = Atb using LAPACK
    std::vector<double> flatA(16, 0.0); // 4x4 matrix, column-major for LAPACK
    for (int col = 0; col < 4; ++col)
    {
      for (int row = 0; row < 4; ++row)
      {
        flatA[col * 4 + row] = AtA[row][col]; // Column-major
      }
    }

    std::vector<double> b = Atb; // copy
    std::vector<int> ipiv(4, 0);

    int n = 4;
    int nrhs = 1;
    int lda = 4;
    int ldb = 4;
    int info = 0;

    LAPACKgesv_(&n, &nrhs, flatA.data(), &lda, ipiv.data(), b.data(), &ldb, &info);

    double value = b[0] + b[1] * cp(0) + b[2] * cp(1) + b[3] * cp(0) * cp(1);

    _var.setNodalValue(value);

    // std::cout << "value = " << value << std::endl;

    // Real value1 = 0;
    // for (_qp = 0; _qp < _qrule->n_points(); _qp++)
    // {
    //   value1 += _JxW[_qp] * _coord[_qp] * _stress[_qp](1, 1);
    //   std::cout << "qp = " << _qp << ", stress = " << _stress[_qp](1, 1) << std::endl;
    // }

    // value1 /= (_bnd ? _current_side_volume : _current_elem_volume);
    // std::cout << "value1 = " << value1 << std::endl;
  }
  else
  {
    Real value = 0;
    for (_qp = 0; _qp < _qrule->n_points(); _qp++)
      value += _JxW[_qp] * _coord[_qp] * _scalar_property[_qp];

    value /= (_bnd ? _current_side_volume : _current_elem_volume);
    _var.setNodalValue(value);
  }
}
