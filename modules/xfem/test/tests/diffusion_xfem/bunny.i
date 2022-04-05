[GlobalParams]
  order = FIRST
  family = LAGRANGE
[]

[XFEM]
  geometric_cut_userobjects = 'cut_mesh'
  qrule = volfrac
  output_cut_plane = true
[]

[Mesh]
  [gen]
    type = GeneratedMeshGenerator
    dim = 3
    nx = 41
    ny = 41
    nz = 41
    xmin = -2
    xmax = 2
    ymin = -2
    ymax = 2
    zmin = -2
    zmax = 2
    elem_type = HEX8
  []
[]

[UserObjects]
  [cut_mesh]
    type = InterfaceMeshCut3DUserObject
    mesh_file = bunny.xda
    interface_velocity_function = vel_func
    heal_always = true
  []
[]

[Functions]
  [vel_func]
    type = ConstantFunction
    value = 0.0
  []
[]

[Variables]
  [u]
  []
[]

[AuxVariables]
  [ls]
  []
[]

[AuxKernels]
  [ls]
    type = MeshCutLevelSetAux
    mesh_cut_user_object = cut_mesh
    variable = ls
    execute_on = 'TIMESTEP_END'
  []
[]

[Kernels]
  [diff]
    type = MatDiffusion
    variable = u
    diffusivity = 1
  []
  [time_deriv]
    type = TimeDerivative
    variable = u
  []
[]

# [Constraints]
#   [u_constraint]
#     type = XFEMEqualValueAtInterface
#     geometric_cut_userobject = 'line_seg_cut_uo'
#     use_displaced_mesh = false
#     variable = u
#     value = 5.1
#     value_neighbor = 0
#     alpha = 10 #0.006
#     level_set_var = phi
#     diff = 1
#     use_penalty = true
#   []
# []

[BCs]
  [front_u]
    type = DirichletBC
    variable = u
    boundary = front
    value = 0
  []
  [back_u]
    type = DirichletBC
    variable = u
    boundary = back
    value = 1
  []
[]

[Executioner]
  type = Transient

  solve_type = 'PJFNK'
  petsc_options_iname = '-pc_type'
  petsc_options_value = 'lu'

  l_max_its = 20
  l_tol = 1e-3
  nl_max_its = 15
  nl_rel_tol = 1e-10
  nl_abs_tol = 1e-10

  start_time = 0.0
  dt = 1
  end_time = 10

  max_xfem_update = 1
[]

[Outputs]
  exodus = true
  execute_on = timestep_end
  csv = true
  perf_graph = true
  [console]
    type = Console
    output_linear = true
  []
[]
