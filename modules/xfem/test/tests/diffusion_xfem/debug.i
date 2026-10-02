[GlobalParams]
  order = FIRST
  family = LAGRANGE
  displacements = 'disp_x disp_y disp_z'
[]

[Mesh]
  [gen]
    type = FileMeshGenerator
    file = iso_3d.e
  []
[]

[XFEM]
  qrule = volfrac
  output_cut_plane = true
[]

[Modules/TensorMechanics/Master]
  [all]
    strain = FINITE
    add_variables = true
    generate_output = 'stress_xx stress_yy stress_zz vonmises_stress'
  []
[]

[UserObjects]
  # [line_seg_cut_uo]
  #   type = LineSegmentCutUserObject
  #   cut_data = '0 0 0 1'
  #   time_start_cut = 0.0
  #   time_end_cut = 0.0
  # []
  [line_seg_cut_uo]
    type = LevelSetCutUserObject
    level_set_var = phi
    heal_always = false
  []
[]

[Functions]
  # [phi_exact]
  #   type = LevelSetOlssonBubble
  #   epsilon = 0.002
  #   center = '0.0 0.0 0.01'
  #   radius = 0.009 #0.0016
  # []
  [phi_exact]
    type = LevelSetOlssonBubble
    epsilon = 0.002
    center = '0.0 0.0 0.01'
    radius = 0.0062 #0.0016
  []
[]

[ICs]
  [phi_ic]
    type = FunctionIC
    function = phi_exact
    variable = phi
  []
[]

[AuxVariables]
  [phi]
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
    boundary = 10
    variable = disp_x
    value = 0.0
  []
  [top_y]
    type = DirichletBC
    boundary = 10
    variable = disp_y
    value = 0.0
  []
  [top_z]
    type = DirichletBC
    boundary = 10
    variable = disp_z
    value = 0.0001
  []

  [bottom_x]
    type = DirichletBC
    boundary = 11
    variable = disp_x
    value = 0.0
  []
  [bottom_y]
    type = DirichletBC
    boundary = 11
    variable = disp_y
    value = 0.0
  []
  [bottom_z]
    type = DirichletBC
    boundary = 11
    variable = disp_z
    value = 0.0
  []
[]

[Executioner]
  type = Transient
  solve_type = 'NEWTON'
  # petsc_options_iname = '-pc_type'
  # petsc_options_value = 'lu'

  petsc_options_iname = '-pc_type  -pc_factor_shift_type -pc_factor_shift_amount'
  petsc_options_value = 'asm      NONZERO               1e-10'

  line_search = 'none'

  l_tol = 1e-3
  nl_max_its = 15
  nl_rel_tol = 1e-10
  nl_abs_tol = 1e-10

  start_time = 0.0
  dt = 2
  end_time = 2
  max_xfem_update = 1

  nl_forced_its = 3
[]

[Outputs]
  csv = true
  interval = 1
  execute_on = timestep_end
  exodus = true
  [console]
    type = Console
    output_linear = true
  []
[]
