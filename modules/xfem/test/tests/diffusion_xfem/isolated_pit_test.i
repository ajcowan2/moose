[GlobalParams]
  order = FIRST
  family = LAGRANGE
[]

[Mesh]
  [gen]
    type = GeneratedMeshGenerator
    dim = 2
    xmin = 0
    xmax = 0.06
    ymin = 0
    ymax = 0.06
    nx = 101
    ny = 101
  []
  [add_bc]
    type = SideSetsFromBoundingBoxGenerator
    input = gen
    block_id = 0
    boundary_id_old = top
    boundary_id_new = 10
    bottom_left = '0.0292 0.055 0'
    top_right = '0.0308 0.065 0'
  []
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
    type = LevelSetOlssonBubble
    epsilon = 0.0005
    center = '0.03 0.03 0'
    radius = 0.008 #0.0016
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
  #   jump = 1
  #   jump_flux = 0
  #   alpha = 1e8
  #   use_penalty = true
  # []
  [u_constraint]
    type = XFEMEqualValueAtInterface
    geometric_cut_userobject = 'line_seg_cut_uo'
    variable = u
    value = 0.0
    value_neighbor = 5.1
    alpha = 1e10
    level_set_var = phi
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
    value = 1
  []
[]

[AuxKernels]
  # [extend_vel]
  #   type = ExtendVelocityLevelSetAux
  #   qp_point_value_user_object = value_uo
  #   variable = ls_vel
  #   execute_on = 'TIMESTEP_END'
  # []
  [extend_vel]
    type = FunctionAux
    function = 0.0001
    variable = ls_vel
    execute_on = 'TIMESTEP_END'
  []
[]

[Executioner]
  type = Transient
  solve_type = 'NEWTON'
  petsc_options_iname = '-pc_type  -pc_factor_shift_type -pc_factor_shift_amount'
  petsc_options_value = 'lu      NONZERO               1e-8'

  line_search = 'none'

  l_tol = 1e-3
  nl_max_its = 15
  nl_rel_tol = 1e-8
  nl_abs_tol = 1e-8

  start_time = 0.0
  dt = 1
  num_steps = 400
  max_xfem_update = 1

  nl_forced_its = 3
[]

# [Postprocessors]
#   [interface_location]
#     type = PositionOfXFEMInterfacePostprocessor
#     value_at_interface_uo = value_uo
#   []
# []

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
