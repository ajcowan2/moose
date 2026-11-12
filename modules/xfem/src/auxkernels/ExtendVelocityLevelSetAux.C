//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "ExtendVelocityLevelSetAux.h"
#include "InterfaceMeshCutUserObjectBase.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace
{
/// Append a debug message to the per-rank file xfem_debug_rank<N>.log (opened once)
void
dbgEmitAux(const unsigned int rank, const std::string & msg)
{
  static std::ofstream f;
  if (!f.is_open())
    f.open("xfem_debug_rank" + std::to_string(rank) + ".log", std::ios::app);
  f << msg << std::flush;
}

/// Format a number in compact scientific notation for the debug output
std::string
dbgFmtAux(const Real v)
{
  std::ostringstream o;
  o << std::scientific << std::setprecision(4) << v;
  return o.str();
}
}

registerMooseObject("XFEMApp", ExtendVelocityLevelSetAux);

InputParameters
ExtendVelocityLevelSetAux::validParams()
{
  InputParameters params = AuxKernel::validParams();
  params.addClassDescription("Extends velocity from an interface to a domain.");

  params.addParam<UserObjectName>(
      "qp_point_value_user_object",
      "Name of QpPointValueAtXFEMInterface that gives values at Qp points along an interface.");

  params.addRequiredParam<PostprocessorName>("diffusivity_positive",
                                             "Postprocessor supplying the positive-side diffusivity");

  params.addRequiredParam<PostprocessorName>("diffusivity_negative",
                                             "Postprocessor supplying the negative-side diffusivity");

  params.addParam<Real>("scale_factor",
                         1.0,
                         "Scalar multiplier applied to the computed velocity");

  params.addRangeCheckedParam<Real>(
      "gb_volume_fraction",
      1.0,
      "gb_volume_fraction > 0 & gb_volume_fraction <= 1",
      "STEFAN only. Volume fraction g of the converted material that is actually Pd2Si, i.e. the "
      "grain-boundary network: the Pd needed per unit volume of advance is g * jump_c, so the "
      "front speed is J / (g * jump_c), which is the grain-boundary enhancement f = 1 / g times "
      "J / jump_c. 1 means the whole volume behind the front converts (previous behavior). "
      "Use the same geometry as the homogenized grain-boundary diffusivity, e.g. "
      "g = 2/3 * q * delta / d.");
  params.addRangeCheckedParam<Real>(
      "stefan_volume_per_Pd",
      0.0,
      "stefan_volume_per_Pd >= 0",
      "STEFAN only. Volume swept by the front per mole of Pd consumed [m^3/mol]; the front speed is "
      "flux * stefan_volume_per_Pd / gb_volume_fraction. 0 (default) uses the Pd2Si product volume, "
      "V_Pd2Si / 2 = 1 / jump_c. To follow the BISON ROM of Bhave et al. (J. Nucl. Mater. 625 (2026) "
      "156536, Eq. 19) use 3 * V_m / (0.5 * pi), with V_m the SiC molar volume.");
  params.addParam<unsigned int>(
      "debug_interval",
      1,
      "With debug = true, only print at time steps that are a multiple of this number.");
  params.addParam<bool>("debug",
                        false,
                        "Print, once per interface point and sweep, all terms that enter the "
                        "velocity (nearest point, map lookups, values, gradients, normal, "
                        "fluxes, diffusivities). Also prints the report of the "
                        "QpPointValueAtXFEMInterface user object when its debug is true.");

  MooseEnum model_type("empirical_correlation INL_ROM Stefan pit_corrosion", "empirical_correlation");
  params.addRequiredParam<MooseEnum>("model_type",
                                     model_type,
                                     "The type of model to apply. Options include: " +
                                         model_type.getRawNames());

  return params;
}

ExtendVelocityLevelSetAux::ExtendVelocityLevelSetAux(const InputParameters & parameters)
  : AuxKernel(parameters),
    _qp_value_uo(getUserObjectByName<QpPointValueAtXFEMInterface>(
        getParam<UserObjectName>("qp_point_value_user_object"))),

    _D_pos(getPostprocessorValue("diffusivity_positive")),
    _D_neg(getPostprocessorValue("diffusivity_negative")),

    _scale_factor(getParam<Real>("scale_factor")),
    _gb_volume_fraction(getParam<Real>("gb_volume_fraction")),
    _stefan_volume_per_Pd(getParam<Real>("stefan_volume_per_Pd")),
    _model_type(getParam<MooseEnum>("model_type").template getEnum<model_type>()),
    _debug(getParam<bool>("debug")),
    _debug_interval(std::max(1u, getParam<unsigned int>("debug_interval")))
{
  if (_debug)
    _console << "[DBG-INIT] " << type() << " '" << name()
             << "': debug output on; per-rank files xfem_debug_rank<N>.log are written in "
             << std::filesystem::current_path().string() << std::endl;
  if (!isNodal())
    mooseError("ExtendVelocityLevelSetAux: Aux variable must be nodal variable.");
}

Real
ExtendVelocityLevelSetAux::computeValue()
{
  _values_positive_level_set_side = _qp_value_uo.getValueAtPositiveLevelSet();
  _values_negative_level_set_side = _qp_value_uo.getValueAtNegativeLevelSet();
  _grad_values_positive_level_set_side = _qp_value_uo.getGradientAtPositiveLevelSet();
  _grad_values_negative_level_set_side = _qp_value_uo.getGradientAtNegativeLevelSet();
  _level_set_normal = _qp_value_uo.getLevelSetNormal();

  _qp_points = _qp_value_uo.getQpPoint();

  unsigned index = 0;
  Real min_dist = std::numeric_limits<Real>::max();
  for (auto const & qp : _qp_points)
  {
    Real dist = (*_current_node - qp.second).norm();
    if (dist < min_dist)
    {
      min_dist = dist;
      index = qp.first;
    }
  }

  // Debug: detect silent failures of the lookups below (std::map::operator[] inserts zeros)
  bool dbg_new_point = false;
  bool miss_pos = false, miss_neg = false, miss_normal = false;
  if (_debug && _t_step % _debug_interval == 0)
  {
    const auto exec_count = _qp_value_uo.getDebugExecCount();
    if (exec_count != _dbg_last_exec_count)
    {
      _dbg_last_exec_count = exec_count;
      _dbg_printed_qps.clear();
      {
        std::ostringstream dbg;
        dbg << "[rank " << processor_id() << "] "
            << "[DBG-AUX] ---- new sweep: t_step=" << _t_step << " t=" << dbgFmtAux(_t)
            << " n_interface_points=" << _qp_points.size() << " (user object exec#" << exec_count
            << ")\n"
            << _qp_value_uo.getDebugReport();
        if (_qp_points.empty())
          dbg << "[rank " << processor_id()
              << "] [DBG-AUX] WARNING: no interface points, every node gets velocity from "
                 "default-constructed (zero) map entries\n";
        dbgEmitAux(processor_id(), dbg.str());
      }
    }
    dbg_new_point = _dbg_printed_qps.insert(index).second;
    miss_pos = !_values_positive_level_set_side.count(index) ||
               !_grad_values_positive_level_set_side.count(index);
    miss_neg = !_values_negative_level_set_side.count(index) ||
               !_grad_values_negative_level_set_side.count(index);
    miss_normal = !_level_set_normal.count(index);
  }

  Real vel = 0.0;
  switch (_model_type)
  {
    case model_type::empirical_correlation:
    {
      Real temperature_avg = 1000; // [K] SiC average temperature
      vel = 38.232 * std::exp(-11342.3 / temperature_avg);
      vel *= 1.0e-6 / (3600.0 * 24.0);
      vel *= _scale_factor;
      break;
    }

    case model_type::INL_ROM:
    {
      Real V_m = 40.096 / 3.21 * 1.0e-6; // [m^3/mol] SiC molar volume

      Real flux_neg = -_D_neg * (_grad_values_negative_level_set_side[index] * _level_set_normal[index]);
      vel = 3 * V_m * flux_neg;

      vel *= _scale_factor;
      break;
    }

    case model_type::Stefan:
    {
      Real V_m_fu   = (2*106.42 + 28.085) / 9.59 * 1.0e-6;  // Pd2Si molar volume [m^3/mol]
      Real n_Pd_fu  = 2.0;                                   // Pd atoms per Pd2Si
      Real jump_c   = n_Pd_fu / V_m_fu;                      // concentration jump [mol/m^3]

      Real flux_neg = -_D_neg * (_grad_values_negative_level_set_side[index] * _level_set_normal[index]);
      // Volume swept per mole of Pd: the Pd2Si product volume unless an input value is given
      const Real volume_per_Pd = _stefan_volume_per_Pd > 0 ? _stefan_volume_per_Pd : 1.0 / jump_c;
      vel = -flux_neg * volume_per_Pd / _gb_volume_fraction;

      vel *= _scale_factor;
      break;
    }

    case model_type::pit_corrosion:
    {
      vel = 0.8102e-5 * _grad_values_positive_level_set_side[index] * _level_set_normal[index] /
             (_values_positive_level_set_side[index] - 143.0);

      vel *= _scale_factor;
      break;
    }

    default:
      mooseError("Invalid model type.");
  }

  if (dbg_new_point)
  {
    // Second nearest interface point, scanned only for the first node of each point and sweep
    Real dbg_min_dist2 = std::numeric_limits<Real>::max();
    unsigned int dbg_index2 = 0;
    for (auto const & qp : _qp_points)
    {
      const Real dist = (*_current_node - qp.second).norm();
      if (qp.first != index && dist < dbg_min_dist2)
      {
        dbg_min_dist2 = dist;
        dbg_index2 = qp.first;
      }
    }

    // Stefan concentration jump, same constants as in the Stefan case above
    const Real jump_c = 2.0 / ((2 * 106.42 + 28.085) / 9.59 * 1.0e-6);
    // Effective Pd per unit volume swept, same choice as in the Stefan case above
    const Real jump_c_eff = _stefan_volume_per_Pd > 0 ? 1.0 / _stefan_volume_per_Pd : jump_c;
    const RealVectorValue n = _level_set_normal[index];
    // Volume fractions of the elements behind this interface point
    const auto & vf_pos_dbg = _qp_value_uo.getPhysicalVolumeFractionAtPositiveLevelSet();
    const auto & vf_neg_dbg = _qp_value_uo.getPhysicalVolumeFractionAtNegativeLevelSet();
    const auto value_or = [](const std::map<unsigned int, Real> & m, const unsigned int k)
    {
      const auto it = m.find(k);
      return it == m.end() ? -1.0 : it->second;
    };
    const Real dbg_vf_pos = value_or(vf_pos_dbg, index);
    const Real dbg_vf_neg = value_or(vf_neg_dbg, index);
    const Real gpn = _grad_values_positive_level_set_side[index] * n;
    const Real gnn = _grad_values_negative_level_set_side[index] * n;
    {
      std::ostringstream dbg;
      dbg << "[rank " << processor_id() << "] ";
      dbg << "[DBG-AUX] node=" << _current_node->id() << " (" << dbgFmtAux((*_current_node)(0))
             << "," << dbgFmtAux((*_current_node)(1)) << ") nearest_qp=" << index
             << " dist=" << dbgFmtAux(min_dist) << " second_qp=" << dbg_index2
             << " dist2=" << dbgFmtAux(dbg_min_dist2) << " | vf(pos,neg)=" << dbgFmtAux(dbg_vf_pos) << "," << dbgFmtAux(dbg_vf_neg)
             << " | missing(pos,neg,normal)=" << miss_pos
             << "," << miss_neg << "," << miss_normal << " | D_pos=" << dbgFmtAux(_D_pos)
             << " D_neg=" << dbgFmtAux(_D_neg) << " scale=" << dbgFmtAux(_scale_factor)
             << " | u_pos=" << dbgFmtAux(_values_positive_level_set_side[index])
             << " u_neg=" << dbgFmtAux(_values_negative_level_set_side[index])
             << " gradu_pos.n=" << dbgFmtAux(gpn) << " gradu_neg.n=" << dbgFmtAux(gnn)
             << " flux_pos=" << dbgFmtAux(-_D_pos * gpn) << " flux_neg=" << dbgFmtAux(-_D_neg * gnn)
             << " jump_c*g=" << dbgFmtAux(jump_c_eff * _gb_volume_fraction) << " | vel_unscaled="
             << dbgFmtAux(_scale_factor != 0 ? vel / _scale_factor : vel)
             << " vel=" << dbgFmtAux(vel) << "\n";
      dbgEmitAux(processor_id(), dbg.str());
    }
  }

  // std::cout << "vel = " << vel << std::endl;
  return vel;
}