[GlobalParams]
  order = FIRST
  family = LAGRANGE
[]

[Mesh]
  [gen]
    type = GeneratedMeshGenerator
    dim = 2
    xmin = 0
    xmax = 0.036
    ymin = 0.018
    ymax = 0.036
    nx = 601
    ny = 301
    elem_type = QUAD4
  []
  [add_bc1]
    type = SideSetsFromBoundingBoxGenerator
    input = gen
    block_id = 0
    boundary_id_old = top
    boundary_id_new = 10
    bottom_left = '0.0109 0.035 0'
    top_right = '0.0131 0.04 0'
  []
  [add_bc2]
    type = SideSetsFromBoundingBoxGenerator
    input = add_bc1
    block_id = 0
    boundary_id_old = top
    boundary_id_new = 11
    bottom_left = '0.0139 0.035 0'
    top_right = '0.0161 0.04 0'
  []
  [add_bc3]
    type = SideSetsFromBoundingBoxGenerator
    input = add_bc2
    block_id = 0
    boundary_id_old = top
    boundary_id_new = 12
    bottom_left = '0.0169 0.035 0'
    top_right = '0.0191 0.04 0'
  []
  [add_bc4]
    type = SideSetsFromBoundingBoxGenerator
    input = add_bc3
    block_id = 0
    boundary_id_old = top
    boundary_id_new = 13
    bottom_left = '0.0199 0.035 0'
    top_right = '0.0221 0.04 0'
  []
  [add_bc5]
    type = SideSetsFromBoundingBoxGenerator
    input = add_bc4
    block_id = 0
    boundary_id_old = top
    boundary_id_new = 14
    bottom_left = '0.0229 0.035 0'
    top_right = '0.0251 0.04 0'
  []
  # [add_bc1]
  #   type = SideSetsFromBoundingBoxGenerator
  #   input = gen
  #   block_id = 0
  #   boundary_id_old = top
  #   boundary_id_new = 10
  #   bottom_left = '0.0106 0.035 0'
  #   top_right = '0.0134 0.04 0'
  # []
  # [add_bc2]
  #   type = SideSetsFromBoundingBoxGenerator
  #   input = add_bc1
  #   block_id = 0
  #   boundary_id_old = top
  #   boundary_id_new = 11
  #   bottom_left = '0.0166 0.035 0'
  #   top_right = '0.0194 0.04 0'
  # []
  # [add_bc3]
  #   type = SideSetsFromBoundingBoxGenerator
  #   input = add_bc2
  #   block_id = 0
  #   boundary_id_old = top
  #   boundary_id_new = 12
  #   bottom_left = '0.0226 0.035 0'
  #   top_right = '0.0254 0.04 0'
  # []
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
    type = LevelSetOlssonBubbles
    epsilon = 0.0003
    centers = '0.018 0.036 0
               0.015 0.036 0
               0.012 0.036 0
               0.021 0.036 0
               0.024 0.036 0'
    radii = '0.0012 0.0012 0.0012 0.0012 0.0012'#0.0016
  []
  # [phi_exact]
  #   type = LevelSetOlssonBubbles
  #   epsilon = 0.0003
  #   centers = '0.018 0.036 0
  #              0.012 0.036 0
  #              0.024 0.036 0'
  #   radii = '0.0016 0.0016 0.0016'#0.0016
  # []
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
  [u_constraint]
    type = XFEMEqualValueAtInterface
    geometric_cut_userobject = 'line_seg_cut_uo'
    use_displaced_mesh = false
    variable = u
    value = 5.1
    value_neighbor = 0
    alpha = 1e6
    level_set_var = phi
  []
  # [u_constraint]
  #   type = XFEMSingleVariableConstraint
  #   geometric_cut_userobject = 'line_seg_cut_uo'
  #   use_displaced_mesh = false
  #   variable = u
  #   use_penalty = true
  #   alpha = 1e6
  # []
[]

[Functions]
  [u_left]
    type = PiecewiseLinear
    x = '0  2'
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
    type = FunctionDirichletBC
    variable = u
    boundary = '10 11 12 13 14'
    function = 0
  []

  # [bottom]
  #   type = DirichletBC
  #   variable = u
  #   boundary = bottom
  #   value = 1
  # []
[]

[MultiApps]
  [update]
    type = TransientMultiApp
    input_files = 'multiple_pits_ls_update.i'
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
  # [extend_vel]
  #   type = FunctionAux
  #   function = 0.0001
  #   variable = ls_vel
  #   execute_on = 'TIMESTEP_END'
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

  l_tol = 1e-3
  nl_max_its = 15
  nl_rel_tol = 1e-10
  nl_abs_tol = 1e-10

  start_time = 0.0
  dt = 1
  end_time = 25
  max_xfem_update = 1

  nl_forced_its = 3
[]


[Outputs]
  csv = true
  interval = 1
  execute_on = timestep_end
  exodus = true
  file_base = five_pits
  [console]
    type = Console
    output_linear = true
  []
[]
