[GlobalParams]
  order = FIRST
  family = LAGRANGE
[]

[Mesh]
  [gen]
    type = GeneratedMeshGenerator
    dim = 3
    xmin = 0
    xmax = 1
    ymin = 0
    ymax = 1
    zmin = 0
    zmax = 1
    nx = 6
    ny = 5
    nz = 5
    elem_type = TET4
  []
[]

[XFEM]
  qrule = volfrac
  output_cut_plane = true
  debug_output_level = 1
[]

[UserObjects]
  [line_seg_cut_uo]
    type = LevelSetCutUserObject
    level_set_var = phi
    heal_always = false
  []
[]

[Functions]
  [phi_exact]
    type = LevelSetOlssonPlane
    epsilon = 0.1
    point = '0.4999999 0.5 0.5'
    normal = '1 0.51 0.15'
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
    alpha = 10 #0.006
    level_set_var = phi
    diff = 1
    use_penalty = false
  []
  # [u_constraint]
  #   type = XFEMSingleVariableConstraint
  #   geometric_cut_userobject = 'line_seg_cut_uo'
  #   use_displaced_mesh = false
  #   variable = u
  #   use_penalty = true
  #   alpha = 1e4
  #   jump = 0
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
    coef = 1
  []
[]

[BCs]
  # Define boundary conditions
  [top]
    type = DirichletBC
    variable = u
    boundary = left
    value = 1
  []

  [bottom]
    type = DirichletBC
    variable = u
    boundary = right
    value = 2
  []
[]

[Executioner]
  type = Transient
  solve_type = 'NEWTON'
  # petsc_options_iname = '-pc_type'
  # petsc_options_value = 'lu'

  # petsc_options_iname = '-pc_type  -pc_factor_shift_type -pc_factor_shift_amount'
  # petsc_options_value = 'lu      NONZERO               1e-10'

  line_search = 'none'

  l_tol = 1e-3
  nl_max_its = 15
  nl_rel_tol = 1e-10
  nl_abs_tol = 1e-10

  start_time = 0.0
  dt = 2
  end_time = 100
  num_steps = 1
  max_xfem_update = 1

  nl_forced_its = 3
[]

[Outputs]
  csv = true
  interval = 1
  execute_on = timestep_end
  exodus = true
  file_base = nitsche
  [console]
    type = Console
    output_linear = true
  []
[]
