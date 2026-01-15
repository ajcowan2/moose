//* This file is part of the MOOSE framework
//* https://mooseframework.inl.gov
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "AuxKernel.h"

class XFEM;
class FEProblemBase;

/**
 * Projects a scalar material property to the kept fragment of cut elements via
 * a bilinear least-squares fit and reports its area-averaged elemental value.
 */
class XFEMScalarOutput : public AuxKernel
{
public:
  static InputParameters validParams();
  XFEMScalarOutput(const InputParameters & parameters);
  virtual ~XFEMScalarOutput() {}

protected:
  virtual Real computeValue() override;
  // In your MOOSE build AuxKernel::compute() returns void.
  virtual void compute() override;

  const MaterialProperty<Real> & _scalar_property;

private:
  std::shared_ptr<XFEM> _xfem;
  FEProblemBase * _fe_problem = nullptr; // used for coord system (RZ) check
  unsigned int _tri_quad_rule = 2;       // triangle integration rule (user param)
};

/** Free helpers declared in header (available to other TUs). */
std::vector<std::array<Point, 3>>
buildPolygon(const XFEM & xfem, const Elem * elem);

void
getqRule(const std::vector<Point> & poly_pts,
         unsigned int tri_rule,
         std::vector<Point> & quad_pts,
         std::vector<Real>  & quad_wts);