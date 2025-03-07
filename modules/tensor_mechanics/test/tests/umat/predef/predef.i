# Testing the UMAT Interface - linear elastic model using the large strain formulation.

[GlobalParams]
  displacements = 'disp_x disp_y disp_z'
[]

[Mesh]
  [gen]
    type = GeneratedMeshGenerator
    dim = 3
    xmin = -0.5
    xmax = 0.5
    ymin = -0.5
    ymax = 0.5
    zmin = -0.5
    zmax = 0.5
  []
[]

[Functions]
  [top_pull]
    type = ParsedFunction
    value = -t*10
  []
[]

[AuxVariables]
  [strain_yy]
    family = MONOMIAL
    order = FIRST
  []
[]

[AuxKernels]
  [strain_yy]
    type = RankTwoAux
    rank_two_tensor = total_strain
    variable = strain_yy
    index_i = 1
    index_j = 1
  []
[]

[Modules/TensorMechanics/Master]
  [all]
    add_variables = true
    strain = FINITE
  []
[]

[BCs]
  [Pressure]
    [bc_presssure]
      boundary = top
      function = top_pull
    []
  []
  [x_bot]
    type = DirichletBC
    variable = disp_x
    boundary = left
    value = 0.0
  []
  [y_bot]
    type = DirichletBC
    variable = disp_y
    boundary = bottom
    value = 0.0
  []
  [z_bot]
    type = DirichletBC
    variable = disp_z
    boundary = front
    value = 0.0
  []
[]

[Materials]
  # Active for
  [umat]
    type = AbaqusUMATStress
    constant_properties = '1000 0.3'
    plugin = '../../../plugins/elastic_predef'
    num_state_vars = 0
    external_fields = 'strain_yy'
  []

  #  2. Active for reference MOOSE computations
  [elasticity_tensor]
    type = ComputeIsotropicElasticityTensor
    base_name = 'base'
    youngs_modulus = 1e3
    poissons_ratio = 0.3
  []
  [strain_dependent_elasticity_tensor]
    type = CompositeElasticityTensor
    args = strain_yy
    tensors = 'base'
    weights = 'prefactor_material'
  []
  [prefactor_material_block]
    type = DerivativeParsedMaterial
    f_name = prefactor_material
    args = strain_yy
    function = '1.0/(1.0 + strain_yy)'
  []
  [stress]
    type = ComputeFiniteStrainElasticStress
  []
[]

[Executioner]
  type = Transient
  solve_type = 'PJFNK'

  petsc_options = '-snes_ksp_ew'
  petsc_options_iname = '-ksp_gmres_restart'
  petsc_options_value = '101'

  line_search = 'none'

  l_max_its = 100
  nl_max_its = 100
  nl_rel_tol = 1e-12
  nl_abs_tol = 1e-10
  l_tol = 1e-9
  start_time = 0.0
  end_time = 30

  dt = 1.0
[]

[Preconditioning]
  [smp]
    type = SMP
    full = true
  []
[]

[Outputs]
  exodus = true
[]
