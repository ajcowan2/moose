[Mesh]
  type = GeneratedMesh
  dim = 2
  xmin = -1
  xmax = 1
  ymin = -1
  ymax = 1
  nx = 121
  ny = 121
[]

[Variables]
  [phi]
  []
[]

[AuxVariables]
  [phi_0]
  []
[]

[BCs]
  [Periodic]
    [all]
      variable = phi
      auto_direction = 'x y'
    []
  []
[]

[Kernels]
  [time]
    type = TimeDerivative
    variable = phi
  []

  [reinit]
    type = LevelSetOlssonReinitialization
    variable = phi
    phi_0 = phi_0
    epsilon = 0.03
    use_modified_reinitilization_formulation = true
  []
[]

[Problem]
  type = LevelSetReinitializationProblem
[]

# [UserObjects]
#   [arnold]
#     type = LevelSetOlssonTerminator
#     tol = 1
#   []
# []

[Executioner]
  type = Transient
  solve_type = PJFNK
  start_time = 0
  num_steps = 10
  nl_rel_tol = 1e-8
  nl_abs_tol = 1e-10
  scheme = crank-nicolson
  petsc_options_iname = '-pc_type -pc_sub_type -ksp_gmres_restart'
  petsc_options_value = 'hypre    boomeramg    300'
  [TimeStepper]
    type = IterationAdaptiveDT
    dt = 0.001
    optimal_iterations = 5
    growth_factor = 5
  []
[]

[Outputs]
[]
