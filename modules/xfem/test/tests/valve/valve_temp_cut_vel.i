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

[XFEM]
  geometric_cut_userobjects = 'cut_mesh'
  qrule = volfrac
  output_cut_plane = true
[]

[Constraints]
  [u_constraint]
    type = XFEMDirichletBC
    geometric_cut_userobject = 'cut_mesh'
    use_displaced_mesh = false
    variable = temperature
    alpha = 1e3
    use_penalty = true
    value = 1
    value_neighbor = 1
    level_set_var = ls
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

[UserObjects]
  [velocity]
    type = XFEMPhaseTransitionMovingInterfaceVelocity
    diffusivity_at_positive_level_set = 5
    diffusivity_at_negative_level_set = 1
    equilibrium_concentration_jump = 1
    value_at_interface_uo = value_uo
  []
  [value_uo]
    type = NodeValueAtXFEMInterface
    variable = 'temperature'
    interface_mesh_cut_userobject = 'cut_mesh'
    execute_on = TIMESTEP_END
    level_set_var = ls
  []
  [cut_mesh]
    type = InterfaceMeshCut2DUserObject
    mesh_file = valve_cut.e
    interface_velocity_uo = velocity
    heal_always = true
    output_exodus = true
  []
[]

[Kernels]
  [diff]
    type = MatDiffusion
    variable = temperature
    diffusivity = 'thermal_conductivity' #14.5e-3 #14.5 (W/mK)
  []
[]

[Materials]
  [thermal_conductivity_1]
    type = GenericFunctionMaterial
    prop_names = thermal_conductivity
    prop_values = 14.5e-3
    block = 1
    outputs = all
  []
  [thermal_conductivity_2]
    type = GenericFunctionMaterial
    prop_names = thermal_conductivity
    prop_values = 14.5e-4
    block = 2
    outputs = all
  []
[]

[BCs]
  # [top]
  #   type = DirichletBC
  #   variable = temperature
  #   boundary = 101
  #   value = 600
  # []
  [bottom]
    type = DirichletBC
    variable = temperature
    boundary = 102
    value = 840
  []
[]

[Executioner]
  type = Transient

  solve_type = 'NEWTON'
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
  end_time = 250

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
