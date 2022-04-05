[Mesh]
  type = GeneratedMesh
  dim = 2
  xmin = -1
  xmax = 1
  ymin = -1
  ymax = 1
  nx = 242
  ny = 242
[]

[AuxVariables]
  [velocity]
    family = LAGRANGE_VEC
  []
  [ls_vel]
    initial_condition = 1
  []
[]

# [XFEM]
#   qrule = volfrac
#   output_cut_plane = true
# []

[UserObjects]
  # [line_seg_cut_uo]
  #   type = LineSegmentCutUserObject
  #   cut_data = '0 0 0 1'
  #   time_start_cut = 0.0
  #   time_end_cut = 0.0
  # []
  # [line_seg_cut_uo]
  #   type = LevelSetCutUserObject
  #   level_set_var = phi
  # []
[]

[Variables]
  [phi]
  []
[]

[Functions]
  [phi_exact]
    type = LevelSetOlssonTwoBubbles
    epsilon = 0.03
    center1 = '-0.3 0.0 0'
    radius1 = 0.149
    center2 = '0.3 0.0 0'
    radius2 = 0.149
  []
  [velocity_func]
    type = ParsedVectorFunction
    value_x = '4*y'
    value_y = '-4*x'
  []
[]

[ICs]
  [phi_ic]
    type = FunctionIC
    function = phi_exact
    variable = phi
  []
  [vel_ic]
    type = VectorFunctionIC
    variable = velocity
    function = velocity_func
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
[]

[Postprocessors]
  [area]
    type = LevelSetVolume
    threshold = 0.5
    variable = phi
    location = outside
    execute_on = 'initial timestep_end'
  []
  [cfl]
    type = LevelSetCFLCondition
    velocity = velocity
    execute_on = 'initial' #timestep_end'
  []
[]

[MultiApps]
  [reinit]
    type = LevelSetReinitializationMultiApp
    input_files = 'reinit.i'
    execute_on = 'timestep_end'
  []
[]

[Transfers]
  [to_sub]
    type = MultiAppCopyTransfer
    variable = phi
    source_variable = phi
    direction = to_multiapp
    multi_app = reinit
    execute_on = 'timestep_end'
  []

  [to_sub_init]
    type = MultiAppCopyTransfer
    variable = phi_0
    source_variable = phi
    direction = to_multiapp
    multi_app = reinit
    execute_on = 'timestep_end'
  []

  [from_sub]
    type = MultiAppCopyTransfer
    variable = phi
    source_variable = phi
    direction = from_multiapp
    multi_app = reinit
    execute_on = timestep_end
  []
[]

[Executioner]
  type = Transient
  solve_type = NEWTON
  start_time = 0
  end_time = 1.570796
  scheme = crank-nicolson
  petsc_options_iname = '-pc_type'
  petsc_options_value = 'lu'
  dt = 0.001
  num_steps = 1000
[]

[Outputs]
  csv = true
  exodus = true
[]
