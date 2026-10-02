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

[Variables]
  [phi]
  []
[]

[AuxVariables]
  [u]
  []
[]

[Functions]
  [phi_left]
    type = PiecewiseLinear
    x = '0   2'
    y = '0  10'
  []
[]

[Kernels]
  [time]
    type = TimeDerivative
    variable = phi
  []
  [diff]
    type = Diffusion
    variable = phi
  []
[]

[BCs]
  # Define boundary conditions
  [left_phi]
    type = FunctionDirichletBC
    variable = phi
    boundary = 3
    function = phi_left
  []

  [right_phi]
    type = DirichletBC
    variable = phi
    boundary = 1
    value = 0
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
