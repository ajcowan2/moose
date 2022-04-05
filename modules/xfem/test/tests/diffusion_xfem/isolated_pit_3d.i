[GlobalParams]
  order = FIRST
  family = LAGRANGE
[]

[Mesh]
  [gen]
    type = FileMeshGenerator
    file = iso_3d.e
  []
[]

[XFEM]
  output_cut_plane = true
  debug_output_level = 1
[]

[UserObjects]
  # [line_seg_cut_uo]
  #   type = LineSegmentCutUserObject
  #   cut_data = '0 0 0 1'
  #   time_start_cut = 0.0
  #   time_end_cut = 0.0
  # []
  [line_seg_cut_uo]
    type = LevelSetCutUserObject
    level_set_var = phi
    heal_always = true
  []
  [value_uo]
    type = QpPointValueAtXFEMInterface
    variable = 'u'
    interface_mesh_cut_userobject = 'line_seg_cut_uo'
    execute_on = TIMESTEP_END
    level_set_var = 'phi'
  []
[]

[Functions]
  [phi_exact]
    type = LevelSetOlssonBubble
    epsilon = 0.002
    center = '0.0 0.0 0.01'
    radius = 0.0041 #0.0016
  []
[]

[ICs]
  [phi_ic]
    type = FunctionIC
    function = phi_exact
    variable = phi
  []
[]

[Variables]
  [u]
    initial_condition = 0
  []
[]

[AuxVariables]
  [phi]

  []
  [ls_vel]
    order = FIRST
    family = LAGRANGE
  []

  # [ls_0]
  #   order = CONSTANT
  #   family = MONOMIAL
  # []
[]

[Constraints]
  [u_constraint]
    type = XFEMEqualValueAtInterface
    geometric_cut_userobject = 'line_seg_cut_uo'
    use_displaced_mesh = false
    variable = u
    value = 5.1
    value_neighbor = 0
    alpha = 0
    level_set_var = phi
    use_penalty = false
    diff = 0.8102e-5
  []
  # [u_constraint]
  #   type = XFEMSingleVariableConstraint
  #   geometric_cut_userobject = 'line_seg_cut_uo'
  #   use_displaced_mesh = false
  #   variable = u
  #   use_penalty = true
  #   alpha = 1e4
  # []
[]

[Functions]
  [u_left]
    type = PiecewiseLinear
    x = '0   2'
    y = '1  2'
  []
[]

[Kernels]
  [diff]
    type = CoefDiffusion
    variable = u
    coef = 0.8102e-5
  []
[]

[BCs]
  # Define boundary conditions
  [top]
    type = DirichletBC
    variable = u
    boundary = 10
    value = 0
  []

  [bottom]
    type = DirichletBC
    variable = u
    boundary = 11
    value = 0
  []
[]

[MultiApps]
  [update]
    type = TransientMultiApp
    input_files = 'isolated_pit_3d_ls_update.i'
    execute_on = 'timestep_end'
    sub_cycling = true
  []
[]

[Transfers]
  [from_sub]
    type = MultiAppNearestNodeTransfer
    source_variable = phi
    variable = phi
    #direction = from_multiapp
    from_multi_app = update
    execute_on = 'timestep_end'
  []
  [to_sub]
    type = MultiAppNearestNodeTransfer
    source_variable = ls_vel
    variable = ls_vel
    #direction = to_multiapp
    to_multi_app = update
    execute_on = 'timestep_end'
  []
[]

[AuxKernels]
  [extend_vel]
    type = ExtendVelocityLevelSetAux
    qp_point_value_user_object = value_uo
    variable = ls_vel
    execute_on = 'TIMESTEP_END'
  []
  # [extend_vel]
  #   type = FunctionAux
  #   function = 0.00031
  #   variable = ls_vel
  #   execute_on = 'TIMESTEP_END'
  # []
  # [component]
  #   type = VariableGradientComponent
  #   component = x
  #   gradient_variable = phi
  #   variable = ls_0
  # []
[]

[Postprocessors]
  [interface_location]
    type = PositionOfXFEMInterfacePostprocessor
    value_at_interface_uo = value_uo
  []
[]

[Executioner]
  type = Transient
  solve_type = 'NEWTON'
  # petsc_options_iname = '-pc_type'
  # petsc_options_value = 'lu'

  petsc_options_iname = '-pc_type  -pc_factor_shift_type -pc_factor_shift_amount'
  petsc_options_value = 'lu      NONZERO               1e-10'

  line_search = 'none'

  l_max_its = 10

  l_tol = 1e-3
  nl_max_its = 15
  nl_rel_tol = 1e-7
  nl_abs_tol = 1e-7

  start_time = 0.0
  dt = 1
  #end_time = 200
  num_steps = 100
  max_xfem_update = 1

  nl_forced_its = 3
[]

[Outputs]
  csv = true
  interval = 1
  execute_on = timestep_end
  exodus = true
  file_base = isolated_pit_3d_test
  [console]
    type = Console
    output_linear = true
  []
[]
