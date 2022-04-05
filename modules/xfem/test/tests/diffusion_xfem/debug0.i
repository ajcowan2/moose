[GlobalParams]
  order = FIRST
  family = LAGRANGE
[]

[Mesh]
  [gen]
    type = GeneratedMeshGenerator
    dim = 3
    xmin = 0
    xmax = 0.03
    ymin = 0
    ymax = 0.03
    zmin = -0.02
    zmax = 0.015
    nx = 9
    ny = 9
    nz = 9
    elem_type = HEX8
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
    center = '0.0 0.0 0.015'
    #radius = 0.0101 #0.0202 #0.0211
    radius = 0.02002
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
    order = FIRST
    family = LAGRANGE
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
    value_neighbor = 4
    alpha = 1
    level_set_var = phi
    diff = 0.8102e-5
    use_penalty = false
  []
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
    type = DirichletBC
    variable = u
    boundary = top
    value = 1
  []

  # [bottom]
  #   type = DirichletBC
  #   variable = u
  #   boundary = bottom
  #   value = 0
  # []
[]

[AuxKernels]
  [phi]
    type = FunctionAux
    function = phi_exact
    variable = phi
    execute_on = 'TIMESTEP_BEGIN'
  []
  # [component]
  #   type = VariableGradientComponent
  #   component = x
  #   gradient_variable = phi
  #   variable = ls_0
  # []
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
  end_time = 1
  max_xfem_update = 1

  nl_forced_its = 3
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
