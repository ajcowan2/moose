[GlobalParams]
  order = FIRST
  family = LAGRANGE
[]

[Mesh]
  [gen]
    type = GeneratedMeshGenerator
    dim = 3
    xmin = -0.015
    xmax = 0.015
    ymin = -0.015
    ymax = 0.015
    zmin = -0.02
    zmax = 0.02
    nx = 32
    ny = 32
    nz = 52
    elem_type = TET4
  []
  # [add_bc]
  #   type = SideSetsFromBoundingBoxGenerator
  #   input = gen
  #   block_id = 0
  #   boundary_id_old = top
  #   boundary_id_new = 10
  #   bottom_left = '0.028 0.055 0'
  #   top_right = '0.032 0.065 0'
  # []
[]

[XFEM]
  qrule = volfrac
  output_cut_plane = true
[]

[UserObjects]
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
    type = LevelSetOlssonSuperellipsoid
    epsilon = 0.0005
    center = '0.0 0.0 0.02'
    a = 0.004
    b = 0.004
    c = 0.012
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
    type = FunctionDirichletBC
    variable = u
    boundary = front
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
    input_files = 'elliptical_pit_ls_update_3d.i'
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
  dt = 4
  end_time = 800
  max_xfem_update = 1

  nl_forced_its = 2
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
