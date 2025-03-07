# Fluid properties
mu = 1
rho = 1
cp = 1
k = 1e-3

# Solid properties
cp_s = 2
rho_s = 4
k_s = 1e-2
h_fs = 10

# Operating conditions
u_inlet = 1
T_inlet = 200
p_outlet = 10
top_side_temperature = 150

# Numerical scheme
advected_interp_method='average'
velocity_interp_method='rc'

[Mesh]
  [gen]
    type = GeneratedMeshGenerator
    dim = 2
    xmin = 0
    xmax = 10
    ymin = 0
    ymax = 1
    nx = 100
    ny = 20
  []
[]

[Variables]
  [u]
    type = PINSFVSuperficialVelocityVariable
    initial_condition = ${u_inlet}
  []
  [v]
    type = PINSFVSuperficialVelocityVariable
    initial_condition = 1e-6
  []
  [pressure]
    type = INSFVPressureVariable
    initial_condition = ${p_outlet}
  []
  [temp_fluid]
    type = INSFVEnergyVariable
  []
  [temp_solid]
    type = MooseVariableFVReal
    initial_condition = 100
  []
[]

[AuxVariables]
  [porosity]
    type = MooseVariableFVReal
    initial_condition = 0.5
  []
[]

[FVKernels]
  [mass]
    type = PINSFVMassAdvection
    variable = pressure
    advected_interp_method = ${advected_interp_method}
    velocity_interp_method = ${velocity_interp_method}
    vel = 'velocity'
    pressure = pressure
    u = u
    v = v
    mu = ${mu}
    rho = ${rho}
    porosity = porosity
  []

  [u_time]
    type = INSFVMomentumTimeDerivative
    variable = u
    rho = ${rho}
  []
  [u_advection]
    type = PINSFVMomentumAdvection
    variable = u
    advected_quantity = 'rhou'
    vel = 'velocity'
    advected_interp_method = ${advected_interp_method}
    velocity_interp_method = ${velocity_interp_method}
    pressure = pressure
    u = u
    v = v
    mu = ${mu}
    rho = ${rho}
    porosity = porosity
  []
  [u_viscosity]
    type = PINSFVMomentumDiffusion
    variable = u
    mu = ${mu}
    porosity = porosity
  []
  [u_pressure]
    type = PINSFVMomentumPressure
    variable = u
    momentum_component = 'x'
    pressure = pressure
    porosity = porosity
  []

  [v_time]
    type = INSFVMomentumTimeDerivative
    variable = v
    rho = ${rho}
  []
  [v_advection]
    type = PINSFVMomentumAdvection
    variable = v
    advected_quantity = 'rhov'
    vel = 'velocity'
    advected_interp_method = ${advected_interp_method}
    velocity_interp_method = ${velocity_interp_method}
    pressure = pressure
    u = u
    v = v
    mu = ${mu}
    rho = ${rho}
    porosity = porosity
  []
  [v_viscosity]
    type = PINSFVMomentumDiffusion
    variable = v
    mu = ${mu}
    porosity = porosity
  []
  [v_pressure]
    type = PINSFVMomentumPressure
    variable = v
    momentum_component = 'y'
    pressure = pressure
    porosity = porosity
  []

  [energy_time]
    type = PINSFVEnergyTimeDerivative
    variable = temp_fluid
    cp = ${cp}
    rho = ${rho}
    is_solid = false
    porosity = porosity
  []
  [energy_advection]
    type = PINSFVEnergyAdvection
    variable = temp_fluid
    vel = 'velocity'
    velocity_interp_method = ${velocity_interp_method}
    advected_interp_method = ${advected_interp_method}
    pressure = pressure
    u = u
    v = v
    mu = ${mu}
    rho = ${rho}
    porosity = porosity
  []
  [energy_diffusion]
    type = PINSFVEnergyDiffusion
    variable = temp_fluid
    k = ${k}
    porosity = porosity
  []
  [energy_convection]
    type = PINSFVEnergyAmbientConvection
    variable = temp_fluid
    is_solid = false
    T_fluid = temp_fluid
    T_solid = temp_solid
    h_solid_fluid = 'h_cv'
  []

  [solid_energy_time]
    type = PINSFVEnergyTimeDerivative
    variable = temp_solid
    cp = ${cp_s}
    rho = ${rho_s}
    is_solid = true
    porosity = porosity
  []
  [solid_energy_diffusion]
    type = FVDiffusion
    variable = temp_solid
    coeff = ${k_s}
  []
  [solid_energy_convection]
    type = PINSFVEnergyAmbientConvection
    variable = temp_solid
    is_solid = true
    T_fluid = temp_fluid
    T_solid = temp_solid
    h_solid_fluid = 'h_cv'
  []
[]

[FVBCs]
  [inlet-u]
    type = INSFVInletVelocityBC
    boundary = 'left'
    variable = u
    function = ${u_inlet}
  []
  [inlet-v]
    type = INSFVInletVelocityBC
    boundary = 'left'
    variable = v
    function = 0
  []
  [inlet-T]
    type = FVNeumannBC
    variable = temp_fluid
    value = ${fparse u_inlet * rho * cp * T_inlet}
    boundary = 'left'
  []

  [no-slip-u]
    type = INSFVNoSlipWallBC
    boundary = 'top'
    variable = u
    function = 0
  []
  [no-slip-v]
    type = INSFVNoSlipWallBC
    boundary = 'top'
    variable = v
    function = 0
  []
  [heated-side]
    type = FVDirichletBC
    boundary = 'top'
    variable = 'temp_solid'
    value = ${top_side_temperature}
  []

  [symmetry-u]
    type = PINSFVSymmetryVelocityBC
    boundary = 'bottom'
    variable = u
    u = u
    v = v
    mu = ${mu}
    momentum_component = 'x'
    porosity = porosity
  []
  [symmetry-v]
    type = PINSFVSymmetryVelocityBC
    boundary = 'bottom'
    variable = v
    u = u
    v = v
    mu = ${mu}
    momentum_component = 'y'
    porosity = porosity
  []
  [symmetry-p]
    type = INSFVSymmetryPressureBC
    boundary = 'bottom'
    variable = pressure
  []

  [outlet-p]
    type = INSFVOutletPressureBC
    boundary = 'right'
    variable = pressure
    function = ${p_outlet}
  []
[]

[Materials]
  [constants]
    type = ADGenericConstantFunctorMaterial
    prop_names = 'h_cv'
    prop_values = '${h_fs}'
  []
  [functor_constants]
    type = ADGenericConstantFunctorMaterial
    prop_names = 'cp'
    prop_values = '${cp}'
  []
  [ins_fv]
    type = INSFVMaterial
    u = 'u'
    v = 'v'
    pressure = 'pressure'
    rho = ${rho}
    temperature = 'temp_fluid'
  []
[]

[Executioner]
  type = Transient
  solve_type = 'NEWTON'
  petsc_options_iname = '-pc_type -ksp_gmres_restart -sub_pc_type -sub_pc_factor_shift_type'
  petsc_options_value = 'asm      100                lu           NONZERO'
  line_search = 'none'
  nl_rel_tol = 1e-12

  end_time = 1.5
[]

# Some basic Postprocessors to examine the solution
[Postprocessors]
  [inlet-p]
    type = SideAverageValue
    variable = pressure
    boundary = 'left'
  []
  [outlet-u]
    type = SideAverageValue
    variable = u
    boundary = 'right'
  []
  [outlet-temp]
    type = SideAverageValue
    variable = temp_fluid
    boundary = 'right'
  []
  [solid-temp]
    type = ElementAverageValue
    variable = temp_solid
  []
[]

[Outputs]
  exodus = true
  csv = false
[]
