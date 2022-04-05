[GlobalParams]
  order = FIRST
  family = LAGRANGE
[]

[Mesh]
  [file]
    type = FileMeshGenerator
    file = valve.e
  []
[]

[Variables]
  [temperature]
  []
[]

[Kernels]
  [diff]
    type = HeatConduction
    variable = temperature
    diffusion_coefficient = 14.5e-3 #14.5 (W/mK)
  []
[]

[BCs]
  [right]
    type = FunctionDirichletBC
    variable = temperature
    boundary = 107
    function = 1
  []
  [bottom]
    type = DirichletBC
    variable = temperature
    boundary = 106
    value = 840
  []
[]

[Executioner]
  type = Transient

  solve_type = 'NEWTON'
  petsc_options_iname = '-pc_type'
  petsc_options_value = 'lu'

  l_max_its = 20
  l_tol = 1e-3
  nl_max_its = 15
  nl_rel_tol = 1e-8
  nl_abs_tol = 1e-8

  start_time = 0.0
  dt = 1
  end_time = 1

  max_xfem_update = 1
[]

[Outputs]
  exodus = true
  #execute_on = 'TIMESTEP_END'
  csv = true
  perf_graph = true
  [console]
    type = Console
    output_linear = true
  []
[]
