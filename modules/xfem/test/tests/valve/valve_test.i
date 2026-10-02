[GlobalParams]
  order = FIRST
  family = LAGRANGE
  #displacements = 'disp_x disp_y'
[]

[XFEM]
  geometric_cut_userobjects = 'cut_mesh'
  qrule = volfrac
  output_cut_plane = true
  #debug_output_level = 3
[]

[Problem]
  kernel_coverage_check = false
[]

[Mesh]
  [file]
    type = FileMeshGenerator
    file = valve.e
  []
[]

[UserObjects]
  [cut_mesh]
    type = InterfaceMeshCut2DUserObject
    mesh_file = valve_cut.e
    interface_velocity_function = 0.117
    #interface_velocity = velocity
    heal_always = true
    block = '2'
  []
[]


[Variables]
  [u]
  []
[]

[AuxVariables]
  [ls]
    order = FIRST
    family = LAGRANGE
  []
[]

[AuxKernels]
  [ls]
    type = MeshCutLevelSetAux
    variable = ls
    mesh_cut_user_object = cut_mesh
  []
[]

[Constraints]
  [u_constraint]
    type = XFEMEqualValueAtInterface
    geometric_cut_userobject = 'cut_mesh'
    use_displaced_mesh = false
    variable = u
    value = 1
    alpha = 1e3
  []
[]

[Kernels]
  [u]
    type = CoefDiffusion
    variable = u
    coef = 100
    block = 1
  []
  [u_new]
    type = CoefDiffusion
    coef = 1
    variable = u
    block = 2
  []
[]

[BCs]
  [fix]
    type = DirichletBC
    variable = u
    boundary = 102
    value = 0
  []
  [bottom]
    type = DirichletBC
    variable = u
    boundary = 106
    value = 2
  []
  [top]
    type = PenaltyDirichletBC
    variable = u
    boundary = 105
    level_set_var = ls
    penalty = 10000
    value = 1
  []
[]

[Executioner]
  type = Transient

  solve_type = 'PJFNK'
  petsc_options_iname = '-pc_type'
  petsc_options_value = 'lu'

  line_search = none

  l_max_its = 20
  l_tol = 1e-3
  nl_max_its = 15
  nl_rel_tol = 1e-8
  nl_abs_tol = 1e-8

  start_time = 0.0
  dt = 1
  end_time = 10

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
