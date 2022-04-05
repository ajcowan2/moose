[Mesh]
  type = GeneratedMesh
  dim = 2
  xmin = 0
  xmax = 0.06
  ymin = 0.03
  ymax = 0.06
  nx = 201
  ny = 101
  elem_type = QUAD4
[]

[AuxVariables]
  [velocity]
    family = LAGRANGE_VEC
  []
  [ls_vel]
    initial_condition = 1
  []
[]

# [XFEM]
#   qrule = volfrac
#   output_cut_plane = true
# []

[UserObjects]
  # [line_seg_cut_uo]
  #   type = LineSegmentCutUserObject
  #   cut_data = '0 0 0 1'
  #   time_start_cut = 0.0
  #   time_end_cut = 0.0
  # []
  # [line_seg_cut_uo]
  #   type = LevelSetCutUserObject
  #   level_set_var = phi
  # []
[]

[Variables]
  [phi]
  []
[]

[Functions]
  [phi_exact]
    type = LevelSetOlssonSuperellipsoid
    epsilon = 0.0005
    center = '0.03 0.06 0'
    a = 0.004
    b = 0.016
    n = 2
  []
[]

[ICs]
  [phi_ic]
    type = FunctionIC
    function = phi_exact
    variable = phi
  []
[]

[Kernels]
  [time]
    type = TimeDerivative
    variable = phi
  []

  [advection]
    type = LevelSetNormalAdvection
    velocity = ls_vel
    variable = phi
  []

  [advection_supg]
    type = LevelSetNormalAdvectionSUPG
    velocity = ls_vel
    variable = phi
  []
  [time_supg]
    type = LevelSetNormalTimeDerivativeSUPG
    velocity = ls_vel
    variable = phi
  []
[]

[Postprocessors]
  [area]
    type = LevelSetVolume
    threshold = 0.5
    variable = phi
    location = outside
    execute_on = 'initial timestep_end'
  []
  [cfl]
    type = LevelSetCFLCondition
    velocity = velocity
    execute_on = 'initial' #timestep_end'
  []
[]

# [Constraints]
#   [xfem_constraint]
#     type = XFEMSingleVariableConstraint
#     variable = phi
#     jump = 0
#     jump_flux = 0
#     use_penalty = true
#     alpha = 1e6
#     geometric_cut_userobject = 'line_seg_cut_uo'
#   []
# []

# [MultiApps]
#   [reinit]
#     type = LevelSetReinitializationMultiApp
#     input_files = 'isolated_reinit.i'
#     execute_on = 'timestep_end'
#   []
# []
#
# [Transfers]
#   [to_sub]
#     type = MultiAppCopyTransfer
#     variable = phi
#     source_variable = phi
#     direction = to_multiapp
#     multi_app = reinit
#     execute_on = 'timestep_end'
#   []
#
#   [to_sub_init]
#     type = MultiAppCopyTransfer
#     variable = phi_0
#     source_variable = phi
#     direction = to_multiapp
#     multi_app = reinit
#     execute_on = 'timestep_end'
#   []
#
#   [from_sub]
#     type = MultiAppCopyTransfer
#     variable = phi
#     source_variable = phi
#     direction = from_multiapp
#     multi_app = reinit
#     execute_on = timestep_end
#   []
# []

[Executioner]
  type = Transient
  solve_type = NEWTON
  start_time = 0
  #end_time = 1.570796
  # scheme = crank-nicolson
  petsc_options_iname = '-pc_type  -pc_factor_shift_type -pc_factor_shift_amount'
  petsc_options_value = 'lu      NONZERO               1e-10'
  nl_rel_tol = 1e-10
  nl_abs_tol = 1e-10
  nl_max_its = 15
  l_max_its = 15
  line_search = 'none'
  dt = 10
  end_time = 800
  nl_forced_its = 3
[]

[Outputs]
  csv = true
  exodus = true
  interval = 1
[]
