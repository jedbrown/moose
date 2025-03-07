diff=1
a=1

[GlobalParams]
  advected_interp_method = 'average'
[]

[Mesh]
  [./gen_mesh]
    type = FileMeshGenerator
    file = skewed.msh
  [../]
[]

[Variables]
  [./v]
    type = MooseVariableFVReal
    face_interp_method = 'skewness-corrected'
    cache_face_gradients = false
    cache_face_values = true
  [../]
[]

[FVKernels]
  [./advection]
    type = FVAdvection
    variable = v
    velocity = '${a} 0 0'
  [../]
  [./diffusion]
    type = FVDiffusion
    variable = v
    coeff = coeff
  [../]
  [./body]
    type = FVBodyForce
    variable = v
    function = 'forcing'
  [../]
[]

[FVBCs]
  [left]
    type = FVFunctionDirichletBC
    boundary = 'left'
    function = 'exact'
    variable = v
  []
  [top]
    type = FVNeumannBC
    boundary = 'top'
    value = 0
    variable = v
  []
  [bottom]
    type = FVNeumannBC
    boundary = 'bottom'
    value = 0
    variable = v
  []
  [right]
    type = FVConstantScalarOutflowBC
    variable = v
    velocity = '${a} 0 0'
    boundary = 'right'
  []
[]

[Materials]
  [diff]
    type = ADGenericConstantFunctorMaterial
    prop_names = 'coeff'
    prop_values = '${diff}'
  []
[]

[Functions]
  [exact]
    type = ParsedFunction
    value = 'cos(x)'
  []
  [forcing]
    type = ParsedFunction
    value = 'cos(x) - sin(x)'
  []
[]

[Executioner]
  type = Steady
  solve_type = 'NEWTON'
  petsc_options_iname = '-pc_type -pc_hypre_type -snes_linesearch_minlambda'
  petsc_options_value = 'hypre boomeramg 1e-9'
[]

[Outputs]
  exodus = true
  csv = true
[]

[Postprocessors]
  [./error]
    type = ElementL2Error
    variable = v
    function = exact
    outputs = 'console csv'
    execute_on = 'timestep_end'
  [../]
  [h]
    type = AverageElementSize
    outputs = 'console csv'
    execute_on = 'timestep_end'
  []
[]
