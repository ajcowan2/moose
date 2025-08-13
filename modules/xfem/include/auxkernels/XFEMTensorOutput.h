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

/**
 * Coupled auxiliary value
 */
class XFEMTensorOutput : public AuxKernel
{
public:
  /**
   * Factory constructor, takes parameters so that all derived classes can be built using the same
   * constructor.
   */
  static InputParameters validParams();

  XFEMTensorOutput(const InputParameters & parameters);

  virtual ~XFEMTensorOutput() {}

protected:
  virtual Real computeValue();
  virtual void compute();
  const MaterialProperty<RankTwoTensor> & _stress;
  unsigned int _i;
  unsigned int _j;

private:
  std::shared_ptr<XFEM> _xfem;
};
