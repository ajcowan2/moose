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
#include "ElementPairLocator.h"
#include "MooseVariableFE.h"
#include "GeometricCutUserObject.h"

class XFEM;
class InterfaceMeshCutUserObjectBase;

class QpPointValueAtXFEMInterface : public GeneralUserObject
{
public:
  static InputParameters validParams();

  QpPointValueAtXFEMInterface(const InputParameters & parameters);

  virtual ~QpPointValueAtXFEMInterface() {}

  virtual void initialize() override;
  virtual void execute() override;
  virtual void finalize() override;

  /**
   * get the map that stores the point index and its values at the positive level set side
   */
  std::map<unsigned int, Real> getValueAtPositiveLevelSet() const
  {
    return _values_positive_level_set_side;
  };

  /**
   * get the map that stores the point index and its values at the negative level set side
   */
  std::map<unsigned int, Real> getValueAtNegativeLevelSet() const
  {
    return _values_negative_level_set_side;
  };

  /**
   * get the map that stores the point index and its gradient at the positive level set side
   */
  std::map<unsigned int, RealVectorValue> getGradientAtPositiveLevelSet() const
  {
    return _grad_values_positive_level_set_side;
  };

  /**
   * get the map that stores the point index and its graident at the negative level set side
   */
  std::map<unsigned int, RealVectorValue> getGradientAtNegativeLevelSet() const
  {
    return _grad_values_negative_level_set_side;
  };

  std::map<unsigned int, RealVectorValue> getLevelSetNormal() const { return _level_set_normal; };

  /**
   * Physical volume fraction of the cut element used on the positive level set side of each
   * interface point (a small value means a sliver element).
   */
  const std::map<unsigned int, Real> & getPhysicalVolumeFractionAtPositiveLevelSet() const
  {
    return _vf_positive_level_set_side;
  }

  /// Same for the element used on the negative level set side
  const std::map<unsigned int, Real> & getPhysicalVolumeFractionAtNegativeLevelSet() const
  {
    return _vf_negative_level_set_side;
  }

  unsigned int numberNodes() const { return _qp_points.size(); };

  std::map<unsigned int, Point> getQpPoint() const { return _qp_points; };

  /// One-line diagnostic text with the interface point counts of the latest execute()
  std::string getDebugReport() const;

  /// Number of execute() calls so far; consumers use it to detect a new evaluation
  unsigned int getDebugExecCount() const { return _dbg_exec_count; }

protected:
  /**
   * Find the element in the element pairs that contains the point in its physical domain.
   * @param p The point in physical space
   * @param positive_level_set True if the physical domain is in positive level set region
   * @return The Elem containing the point or NULL if this processor doesn't contain an element that
   * contains this point.
   */
  const Elem * getElemContainingPoint(const Point & p, bool positive_level_set);

  /**
   * Debug only: dump the nodes of a side element (sliver, large gradient or reference) used at an
   * interface point. Must be called right after reinitElemPhys(elem, {p}).
   * @param i Interface point index
   * @param p The interface point
   * @param elem The element used on this side
   * @param positive_side True if elem is the positive level set side element
   */
  void debugDumpElem(const unsigned int i,
                     const Point & p,
                     const Elem & elem,
                     const bool positive_side);

  /// The computation mesh
  MooseMesh & _mesh;

  /// The qp points to evaluate at
  std::map<unsigned int, Point> _qp_points;

  /// Pointer to PointLocatorBase object
  std::unique_ptr<PointLocatorBase> _pl;

  /// Pointer to the XFEM controller object
  std::shared_ptr<XFEM> _xfem;

  /// Pointer to ElementPairList object
  const ElementPairLocator::ElementPairList * _elem_pairs;

  /// Pointer to LineSegmentCutSetUserObject object
  const GeometricCutUserObject * _mesh_cut;

  /// Pointer to MooseVariableFEBase object
  MooseVariableFEBase * _var;

  /// Pointer to MooseVariableFEBase object
  MooseVariableFEBase * _var_level_set;

  /// The variable number of the level set variable we are operating on
  const unsigned int _level_set_var_number;

  /// System reference
  const System & _system;

  /// The subproblem solution vector
  const NumericVector<Number> & _solution;

  /// System of the sampled variable
  const System & _u_system;

  /// Solution vector of the system of the sampled variable (debug dumps of nodal values)
  const NumericVector<Number> & _u_solution;

  /// Mapping from point index and its values at the positive level set side
  std::map<unsigned int, Real> _values_positive_level_set_side;

  /// Mapping from point index and its values at the negative level set side
  std::map<unsigned int, Real> _values_negative_level_set_side;

  /// Mapping from point index and its gradient at the positive level set side
  std::map<unsigned int, RealVectorValue> _grad_values_positive_level_set_side;

  /// Mapping from point index and its gradient at the negative level set side
  std::map<unsigned int, RealVectorValue> _grad_values_negative_level_set_side;

  /// Mapping from point index and its gradient at the negative level set side
  std::map<unsigned int, RealVectorValue> _level_set_normal;

  /// Mapping from point index to the physical volume fraction of its positive side element
  std::map<unsigned int, Real> _vf_positive_level_set_side;

  /// Mapping from point index to the physical volume fraction of its negative side element
  std::map<unsigned int, Real> _vf_negative_level_set_side;

  /// Print diagnostics of the interface point sampling (see debug parameter)
  const bool _debug;

  /// Side elements with a physical volume fraction below this get a [DBG-SLV] node dump
  const Real _dbg_sliver_fraction;

  /// Non-sliver side elements with |grad_u.n| above this are dumped too
  const Real _dbg_gradient_threshold;

  /// Number of well-resolved reference elements dumped per execute()
  const unsigned int _dbg_n_reference;

  /// Maximum number of [DBG-SLV] dumps per execute() (limits the log size)
  const std::size_t _dbg_max_slv_records = 12;

  /// [DBG-SLV] dumps of the latest execute()
  std::vector<std::string> _dbg_slv_records;

  /// Reference dumps done in the latest execute()
  unsigned int _dbg_n_reference_done = 0;

  ///@{ Segment length and geometric normal of the cut of each interface point (debug only)
  std::map<unsigned int, Real> _dbg_seg_length;
  std::map<unsigned int, Point> _dbg_geom_normal;
  std::vector<Real> _dbg_seg_length_vector;
  std::vector<Point> _dbg_geom_normal_vector;
  ///@}

  /// Number of execute() calls so far
  unsigned int _dbg_exec_count = 0;

  ///@{ Debug counters of the latest execute(): points and points without a value per side
  unsigned int _dbg_n_qp = 0;
  unsigned int _dbg_local_pos_missing = 0;
  unsigned int _dbg_local_neg_missing = 0;
  unsigned int _dbg_global_pos_missing = 0;
  unsigned int _dbg_global_neg_missing = 0;
  ///@}
};
