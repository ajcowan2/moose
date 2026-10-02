[GlobalParams]
  order = FIRST
  family = LAGRANGE
[]

[Mesh]
  [gen]
    type = GeneratedMeshGenerator
    dim = 2
    nx = 11
    ny = 1
    xmin = 0.0
    xmax = 20.0
    ymin = 0.0
    ymax = 5.0
    elem_type = QUAD4
  []
[]

[XFEM]
  qrule = volfrac
  output_cut_plane = true
[]

[UserObjects]
  [cut_mesh]
    type = InterfaceMeshCut2DUserObject
    mesh_file = flat_interface_1d.e
    interface_velocity_function = 1.1
    heal_always = true
  []
  [value_uo]
    type = QpPointValueAtXFEMInterface
    variable = 'u'
    interface_mesh_cut_userobject = 'cut_mesh'
    execute_on = TIMESTEP_END
    level_set_var = ls
  []
[]

[Variables]
  [u]
  []
[]

[ICs]
  [ic_u]
    type = FunctionIC
    variable = u
    function = 'if(x<5.01, 2, 1)'
  []
[]

[AuxVariables]
  [ls]
    order = FIRST
    family = LAGRANGE
  []
  [ls_vel]
    order = FIRST
    family = LAGRANGE
  []
[]

[Constraints]
  [u_constraint]
    type = XFEMEqualValueAtInterface
    geometric_cut_userobject = 'cut_mesh'
    use_displaced_mesh = false
    variable = u
    value = 2
    alpha = 1e6
  []
[]

[Kernels]
  [diff]
    type = MatDiffusion
    variable = u
    diffusivity = diffusion_coefficient
  []
  [time]
    type = TimeDerivative
    variable = u
  []
[]

[AuxKernels]
  [ls]
    type = MeshCutLevelSetAux
    mesh_cut_user_object = cut_mesh
    variable = ls
    execute_on = 'TIMESTEP_BEGIN'
  []
  [extend_vel]
    type = ExtendVelocityLevelSetAux
    qp_point_value_user_object = value_uo
    variable = ls_vel
  []
[]

[Materials]
  [diffusivity_A]
    type = GenericConstantMaterial
    prop_names = A_diffusion_coefficient
    prop_values = 5
  []
  [diffusivity_B]
    type = GenericConstantMaterial
    prop_names = B_diffusion_coefficient
    prop_values = 1
  []
  [diff_combined]
    type = LevelSetBiMaterialReal
    levelset_positive_base = 'A'
    levelset_negative_base = 'B'
    level_set_var = ls
    prop_name = diffusion_coefficient
  []
[]

[BCs]
  # Define boundary conditions
  [left_u]
    type = DirichletBC
    variable = u
    value = 2
    boundary = left
  []

  [right_u]
    type = NeumannBC
    variable = u
    boundary = right
    value = 0
  []
[]

[Executioner]
  type = Transient
  solve_type = 'PJFNK'
  petsc_options_iname = '-pc_type'
  petsc_options_value = 'lu'
  line_search = 'none'

  l_tol = 1e-3
  nl_max_its = 15
  nl_rel_tol = 1e-9
  nl_abs_tol = 1e-9

  start_time = 0.0
  dt = 1
  num_steps = 5
  max_xfem_update = 1
[]

[Outputs]
  execute_on = timestep_end
  exodus = true
  perf_graph = true
  csv = true
[]
