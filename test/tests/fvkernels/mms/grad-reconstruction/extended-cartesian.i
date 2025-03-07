a=1.1
diff=1.1

[Mesh]
  [./gen_mesh]
    type = GeneratedMeshGenerator
    dim = 2
    xmin = 0
    xmax = 1
    ymin = 0
    ymax = 1
    nx = 2
    ny = 2
  [../]
[]

[Problem]
  kernel_coverage_check = off
[]

[Variables]
  [./v]
    family = MONOMIAL
    order = CONSTANT
    fv = true
    initial_condition = 1
    type = MooseVariableFVReal
    face_interp_method = 'vertex-based'
  [../]
[]

[FVKernels]
  [./advection]
    type = FVElementalAdvection
    variable = v
    velocity = '${a} ${fparse 2 * a} 0'
    # going to request gradient reconstruction with an extended stencil
    use_point_neighbors = true
  []
  [reaction]
    type = FVReaction
    variable = v
  []
  [diff_v]
    type = FVDiffusion
    variable = v
    coeff = ${diff}
  []
  [body_v]
    type = FVBodyForce
    variable = v
    function = 'forcing'
  []
[]

[FVBCs]
  [diri]
    type = FVFunctionDirichletBC
    boundary = 'left right top bottom'
    function = 'exact'
    variable = v
  []
[]

[Functions]
[exact]
  type = ParsedFunction
  value = 'sin(x)*cos(y)'
[]
[forcing]
  type = ParsedFunction
  value = '-2*a*sin(x)*sin(y) + a*cos(x)*cos(y) + 2*diff*sin(x)*cos(y) + sin(x)*cos(y)'
  vars = 'a diff'
  vals = '${a} ${diff}'
[]
[]

[Executioner]
  type = Steady
  solve_type = 'NEWTON'
  petsc_options_iname = '-pc_type -sub_pc_factor_shift_type -sub_pc_type'
  petsc_options_value = 'asm      NONZERO                   lu'
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
