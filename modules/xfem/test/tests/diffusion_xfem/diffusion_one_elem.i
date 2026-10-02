[GlobalParams]
  order = FIRST
  family = LAGRANGE
[]

[Mesh]
  type = GeneratedMesh
  dim = 2
  xmin = -1
  xmax = 1
  ymin = -1
  ymax = 1
  nx = 1
  ny = 1
[]

[XFEM]
  qrule = volfrac
  output_cut_plane = true
[]

[UserObjects]
  [line_seg_cut_uo]
    type = LineSegmentCutUserObject
    cut_data = '0 -1 0 1'
    time_start_cut = 0.0
    time_end_cut = 0.0
  []
[]

[Variables]
  [u]
  []
[]

[AuxVariables]
  [phi]
  []
[]

[Functions]
  [u_left]
    type = PiecewiseLinear
    x = '0   2'
    y = '0  0.1'
  []
[]

[Kernels]
  [time]
    type = TimeDerivative
    variable = u
  []
  [diff]
    type = Diffusion
    variable = u
  []
[]

[BCs]
  # Define boundary conditions
  [left_u]
    type = FunctionDirichletBC
    variable = u
    boundary = 3
    function = u_left
  []

  [right_u]
    type = DirichletBC
    variable = u
    boundary = 1
    value = 0
  []
[]

[MultiApps]
  [reinit]
    type = TransientMultiApp
    input_files = 'ls_one_elem.i'
    execute_on = 'timestep_end'
  []
[]

[Transfers]
  # [from_sub]
  #   type = MultiAppNearestNodeTransfer
  #   source_variable = phi
  #   variable = phi
  #   direction = from_multiapp
  #   multi_app = reinit
  #   execute_on = 'timestep_end'
  # []
  [to_sub]
    type = MultiAppNearestNodeTransfer
    source_variable = u
    variable = u
    direction = to_multiapp
    multi_app = reinit
    execute_on = 'timestep_end'
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
  dt = 0.002
  num_steps = 1
  max_xfem_update = 1
[]

[Outputs]
  interval = 1
  execute_on = timestep_end
  exodus = true
  [console]
    type = Console
    output_linear = true
  []
[]
