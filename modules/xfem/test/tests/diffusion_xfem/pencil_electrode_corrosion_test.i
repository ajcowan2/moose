[GlobalParams]
  order = FIRST
  family = LAGRANGE
[]

[Mesh]
  type = GeneratedMesh
  dim = 2
  xmin = 0
  xmax = 0.016
  ymin = 0
  ymax = 0.016
  nx = 101
  ny = 101
  # elem_type = TRI3
[]

[XFEM]
  qrule = volfrac
  output_cut_plane = true
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
    level_set_var = phi
  []
[]

[Functions]
  [phi_exact]
    type = LevelSetOlssonPlane
    epsilon = 0.0005
    point = '0 0.0157 0'
    #point = '0 0.01 0'
    normal = '0 1 0'
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
[]

[Constraints]
  # [u_constraint]
  #   type = XFEMSingleVariableConstraint
  #   geometric_cut_userobject = 'line_seg_cut_uo'
  #   use_displaced_mesh = false
  #   variable = u
  #   jump = 0
  #   jump_flux = 0
  #   alpha = 1e6
  #   use_penalty = true
  # []
  [u_constraint]
    type = XFEMEqualValueAtInterface
    geometric_cut_userobject = 'line_seg_cut_uo'
    use_displaced_mesh = false
    variable = u
    value = 143
    value_neighbor = 5.1
    alpha = 1e6
  []
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
    coef = 0.824e-5
  []
[]

[BCs]
  # Define boundary conditions
  [top]
    type = FunctionDirichletBC
    variable = u
    boundary = top
    function = 0
  []

  [bottom]
    type = DirichletBC
    variable = u
    boundary = bottom
    value = 143
  []
[]

[MultiApps]
  [update]
    type = TransientMultiApp
    input_files = 'corrosion_ls_update.i'
    execute_on = 'timestep_end'
  []
[]

[Transfers]
  [from_sub]
    type = MultiAppNearestNodeTransfer
    source_variable = phi
    variable = phi
    direction = from_multiapp
    multi_app = update
    execute_on = 'timestep_end'
  []
  [to_sub]
    type = MultiAppNearestNodeTransfer
    source_variable = ls_vel
    variable = ls_vel
    direction = to_multiapp
    multi_app = update
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
[]

[Executioner]
  type = Transient
  solve_type = 'NEWTON'
  petsc_options_iname = '-pc_type'
  petsc_options_value = 'lu'
  line_search = 'none'

  l_tol = 1e-3
  nl_max_its = 15
  nl_rel_tol = 1e-10
  nl_abs_tol = 1e-10

  start_time = 0.0
  dt = 1
  num_steps = 400
  max_xfem_update = 1

  nl_forced_its = 3
[]

[Postprocessors]
  [interface_location]
    type = PositionOfXFEMInterfacePostprocessor
    value_at_interface_uo = value_uo
  []
[]

[Outputs]
  csv = true
  interval = 1
  execute_on = timestep_end
  exodus = true
  [console]
    type = Console
    output_linear = true
  []
[]
