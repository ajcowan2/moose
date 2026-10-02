[GlobalParams]
  order = FIRST
  family = LAGRANGE
  displacements = 'disp_x disp_y disp_z'
[]

[Mesh]
  type = GeneratedMesh
  dim = 3
  xmin = 0
  xmax = 0.03
  ymin = 0
  ymax = 0.03
  zmin = -0.02
  zmax = 0.015
  nx = 14
  ny = 14
  nz = 14
  elem_type = HEX8
[]

# [Mesh]
#   [gen]
#     type = FileMeshGenerator
#     file = iso_test_2.e
#   []
# []

[XFEM]
  qrule = volfrac
  output_cut_plane = true
  debug_output_level = 1
[]

[Modules/TensorMechanics/Master]
  [all]
    strain = FINITE
    add_variables = true
    generate_output = 'stress_xx stress_yy stress_zz vonmises_stress'
  []
[]

[UserObjects]
  [line_seg_cut_uo]
    type = LevelSetCutUserObject
    level_set_var = phi
    heal_always = false
  []
[]

[Functions]
  [phi_exact]
    type = LevelSetOlssonBubble
    epsilon = 0.002
    center = '0.0 0.0 0.015'
    radius = 0.02021 #3 0.0202 #2 0.02013 #0.02023
  []
[]

[AuxVariables]
  [phi]
  []
[]

[ICs]
  [phi_ic]
    type = FunctionIC
    function = phi_exact
    variable = phi
  []
[]


[Materials]
  [elasticity_tensor]
    type = ComputeIsotropicElasticityTensor
    youngs_modulus = 207000
    poissons_ratio = 0.3
  []
  [stress]
    type = ComputeFiniteStrainElasticStress
  []
[]

[BCs]
  [top_x]
    type = DirichletBC
    boundary = front #10 #front
    variable = disp_x
    value = 0.0
  []
  [top_y]
    type = DirichletBC
    boundary = front #10 #front
    variable = disp_y
    value = 0.0
  []
  [top_z]
    type = FunctionDirichletBC
    boundary = front #10 #front
    variable = disp_z
    function = 0.0001
  []

  [bottom_x]
    type = DirichletBC
    boundary = back #11 #back
    variable = disp_x
    value = 0.0
  []
  [bottom_y]
    type = DirichletBC
    boundary = back #11 #back
    variable = disp_y
    value = 0.0
  []
  [bottom_z]
    type = DirichletBC
    boundary = back #11 #back
    variable = disp_z
    value = 0.0
  []
[]

[Executioner]
  type = Transient
  solve_type = 'NEWTON'
  petsc_options_iname = '-pc_type'
  petsc_options_value = 'lu'

  # petsc_options_iname = '-pc_type  -pc_factor_shift_type -pc_factor_shift_amount'
  # petsc_options_value = 'asm      NONZERO               1e-10'

  line_search = 'none'

  l_tol = 1e-3
  nl_max_its = 15
  nl_rel_tol = 1e-10
  nl_abs_tol = 1e-10

  start_time = 0.0
  dt = 1
  end_time = 1
  max_xfem_update = 1

  nl_forced_its = 3
[]

[Outputs]
  csv = true
  interval = 1
  execute_on = timestep_end
  exodus = true
  file_base = debug31
  [console]
    type = Console
    output_linear = true
  []
[]
