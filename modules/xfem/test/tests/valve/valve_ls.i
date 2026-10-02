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
  qrule = volfrac
  output_cut_plane = true
[]

[Constraints]
  [u_constraint]
    type = XFEMDirichletBC
    geometric_cut_userobject = 'line_seg_cut_uo'
    use_displaced_mesh = false
    variable = temperature
    alpha = 1e3
    use_penalty = true
    value = 1
    value_neighbor = 1
    level_set_var = phi
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
[]

[AuxKernels]
  [phi]
    type = MeshCutLevelSetAux
    variable = phi
    mesh_cut_user_object = cut_mesh
    execute_on = INITIAL
  []
  [extend_vel]
    type = ExtendVelocityLevelSetAux
    qp_point_value_user_object = value_uo
    variable = ls_vel
    execute_on = 'TIMESTEP_END'
  []
[]

[UserObjects]
  [cut_mesh]
    type = InterfaceMeshCut2DUserObject
    mesh_file = valve_cut.e
    interface_velocity_function = 0
    heal_always = true
    output_exodus = true
  []
  [line_seg_cut_uo]
    type = LevelSetCutUserObject
    level_set_var = phi
    heal_always = true
  []
  [value_uo]
    type = QpPointValueAtXFEMInterface
    variable = 'temperature'
    interface_mesh_cut_userobject = 'line_seg_cut_uo'
    execute_on = TIMESTEP_END
    level_set_var = 'phi'
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

[MultiApps]
  [update]
    type = TransientMultiApp
    input_files = 'valve_ls_update.i'
    execute_on = 'timestep_end'
    sub_cycling = true
  []
[]

[Transfers]
  [from_sub]
    type = MultiAppNearestNodeTransfer
    source_variable = phi
    variable = phi
    #direction = from_multiapp
    from_multi_app = update
    execute_on = 'timestep_end'
  []
  [to_sub]
    type = MultiAppNearestNodeTransfer
    source_variable = ls_vel
    variable = ls_vel
    #direction = to_multiapp
    to_multi_app = update
    execute_on = 'timestep_end'
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
  file_base = valve_ls_fine
  #execute_on = 'TIMESTEP_END'
  csv = true
  perf_graph = true
  [console]
    type = Console
    output_linear = true
  []
[]
