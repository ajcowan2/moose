[GlobalParams]
  order = FIRST
  family = LAGRANGE
  #displacements = 'disp_x disp_y'
[]

[Problem]
  kernel_coverage_check = false
[]

[XFEM]
  geometric_cut_userobjects = 'cut_mesh'
  qrule = volfrac
  output_cut_plane = true
  #debug_output_level = 3
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
  [bottom]
    type = SubdomainBoundingBoxGenerator
    input = generated_mesh
    block_id = 0
    bottom_left = '0 0 0'
    top_right = '10 55 0'
  []
  [top]
    type = SubdomainBoundingBoxGenerator
    input = bottom
    block_id = 1
    bottom_left = '10 0 0'
    top_right = '20 55 0'
  []
[]

[Constraints]
  [u_constraint]
    type = XFEMEqualValueAtInterface
    geometric_cut_userobject = 'cut_mesh'
    use_displaced_mesh = false
    variable = u
    value = 2
    alpha = 1e3
  []
[]

[UserObjects]
  [cut_mesh]
    type = InterfaceMeshCut2DUserObject
    mesh_file = valve_exterior.e
    interface_velocity_function = 0.05
    #interface_velocity = velocity
    heal_always = true
    # block = '2'
  []
  [esm]
    type = CutElementSubdomainModifier
    geometric_cut_userobject = cut_mesh
    apply_initial_conditions = false
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

[Kernels]
  [u]
    type = Diffusion
    variable = u
  []
[]

[BCs]
  [bottom]
    type = DirichletBC
    variable = u
    boundary = bottom
    value = 2
  []
  [top]
    type = DirichletBC
    variable = u
    boundary = right
    value = 10
  []
[]

[Executioner]
  type = Transient

  solve_type = 'NEWTON'
  petsc_options_iname = '-pc_type'
  petsc_options_value = 'lu'

  line_search = 'none'

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
  # execute_on = 'TIMESTEP_BEGIN'
  csv = true
  perf_graph = true
  [console]
    type = Console
    output_linear = true
  []
[]
