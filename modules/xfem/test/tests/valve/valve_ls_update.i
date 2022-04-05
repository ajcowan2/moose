[Mesh]
  [gen]
    type = FileMeshGenerator
    file = valve.e
  []
[]

[AuxVariables]
  [velocity]
    family = LAGRANGE_VEC
  []
  [ls_vel]
    initial_condition = 1
  []
[]

[Variables]
  [phi]
  []
[]

[ICs]
  [valve_ic]
    type = ValveLevelSetIC
    mesh_file = valve_cut.e
    variable = phi
  []
[]

[Kernels]
  [time]
    type = TimeDerivative
    variable = phi
  []

  [advection]
    type = LevelSetNormalAdvection
    velocity = ls_vel
    variable = phi
  []

  [advection_supg]
    type = LevelSetNormalAdvectionSUPG
    velocity = ls_vel
    variable = phi
  []
  [time_supg]
    type = LevelSetNormalTimeDerivativeSUPG
    velocity = ls_vel
    variable = phi
  []
[]

[Executioner]
  type = Transient
  solve_type = NEWTON
  start_time = 0
  #end_time = 1.570796
  #scheme = crank-nicolson
  petsc_options_iname = '-pc_type  -pc_factor_shift_type -pc_factor_shift_amount'
  petsc_options_value = 'lu      NONZERO               1e-10'
  nl_rel_tol = 1e-8
  nl_abs_tol = 1e-8
  nl_max_its = 15
  l_max_its = 15
  line_search = 'none'
  dt = 0.1
  num_steps = 5
  end_time = 800
  nl_forced_its = 3
[]

[Outputs]
  csv = true
  exodus = true
  interval = 1
[]
