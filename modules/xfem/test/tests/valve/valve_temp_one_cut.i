[GlobalParams]
  order = FIRST
  family = LAGRANGE
[]

[Mesh]
  [generated_mesh]
    type = GeneratedMeshGenerator
    dim = 2
    nx = 21
    ny = 61
    xmin = 0
    xmax = 20
    ymin = 0
    ymax = 55
    elem_type = QUAD4
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
    type = XFEMSingleVariableConstraint
    geometric_cut_userobject = 'cut_mesh'
    use_displaced_mesh = false
    variable = temperature
    jump = 0
    alpha = 1e3
    use_penalty = true
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
  [cut_mesh]
    type = InterfaceMeshCut2DUserObject
    mesh_file = valve_cut.e
    interface_velocity_function = 0.13
    heal_always = true
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
  # [right]
  #   type = FunctionDirichletBC
  #   variable = temperature
  #   boundary = 107
  #   function = 1
  # []
  [bottom]
    type = DirichletBC
    variable = temperature
    boundary = bottom
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
  end_time = 25

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
