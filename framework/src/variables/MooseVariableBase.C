//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "MooseVariableBase.h"

#include "AddVariableAction.h"
#include "SubProblem.h"
#include "SystemBase.h"
#include "MooseMesh.h"
#include "MooseApp.h"
#include "InputParameterWarehouse.h"

#include "libmesh/variable.h"
#include "libmesh/dof_map.h"
#include "libmesh/system.h"
#include "libmesh/fe_type.h"
#include "libmesh/string_to_enum.h"

// Users should never actually create this object
registerMooseObject("MooseApp", MooseVariableBase);

defineLegacyParams(MooseVariableBase);

InputParameters
MooseVariableBase::validParams()
{
  InputParameters params = MooseObject::validParams();
  params += BlockRestrictable::validParams();
  params += OutputInterface::validParams();

  MooseEnum order(
      "CONSTANT FIRST SECOND THIRD FOURTH FIFTH SIXTH SEVENTH EIGHTH NINTH TENTH ELEVENTH TWELFTH "
      "THIRTEENTH FOURTEENTH FIFTEENTH SIXTEENTH SEVENTEENTH EIGHTTEENTH NINETEENTH TWENTIETH "
      "TWENTYFIRST TWENTYSECOND TWENTYTHIRD TWENTYFOURTH TWENTYFIFTH TWENTYSIXTH TWENTYSEVENTH "
      "TWENTYEIGHTH TWENTYNINTH THIRTIETH THIRTYFIRST THIRTYSECOND THIRTYTHIRD THIRTYFOURTH "
      "THIRTYFIFTH THIRTYSIXTH THIRTYSEVENTH THIRTYEIGHTH THIRTYNINTH FORTIETH FORTYFIRST "
      "FORTYSECOND FORTYTHIRD",
      "FIRST",
      true);
  params.addParam<MooseEnum>("order",
                             order,
                             "Order of the FE shape function to use for this variable (additional "
                             "orders not listed here are allowed, depending on the family).");

  MooseEnum family{AddVariableAction::getNonlinearVariableFamilies()};

  params.addParam<MooseEnum>(
      "family", family, "Specifies the family of FE shape functions to use for this variable.");

  // ArrayVariable capability
  params.addRangeCheckedParam<unsigned int>(
      "components", 1, "components>0", "Number of components for an array variable");

  // Advanced input options
  params.addParam<std::vector<Real>>("scaling",
                                     "Specifies a scaling factor to apply to this variable");
  params.addParam<bool>("eigen", false, "True to make this variable an eigen variable");
  params.addParam<bool>("fv", false, "True to make this variable a finite volume variable");
  params.addParamNamesToGroup("scaling eigen", "Advanced");

  params.addParam<bool>("use_dual", false, "True to use dual basis for Lagrange multipliers");

  params.registerBase("MooseVariableBase");
  params.addPrivateParam<SystemBase *>("_system_base");
  params.addPrivateParam<FEProblemBase *>("_fe_problem_base");
  params.addPrivateParam<Moose::VarKindType>("_var_kind");
  params.addPrivateParam<unsigned int>("_var_num");
  params.addPrivateParam<THREAD_ID>("tid");

  params.addClassDescription(
      "Base class for Moose variables. This should never be the terminal object type");
  return params;
}

MooseVariableBase::MooseVariableBase(const InputParameters & parameters)
  : MooseObject(parameters),
    BlockRestrictable(this),
    OutputInterface(parameters),
    SetupInterface(this),
    _sys(*getParam<SystemBase *>("_system_base")), // TODO: get from _fe_problem_base
    _fe_type(Utility::string_to_enum<Order>(getParam<MooseEnum>("order")),
             Utility::string_to_enum<FEFamily>(getParam<MooseEnum>("family"))),
    _var_num(getParam<unsigned int>("_var_num")),
    _is_eigen(getParam<bool>("eigen")),
    _var_kind(getParam<Moose::VarKindType>("_var_kind")),
    _subproblem(_sys.subproblem()),
    _variable(_sys.system().variable(_var_num)),
    _assembly(_subproblem.assembly(getParam<THREAD_ID>("_tid"))),
    _dof_map(_sys.dofMap()),
    _mesh(_subproblem.mesh()),
    _tid(getParam<THREAD_ID>("tid")),
    _count(getParam<unsigned int>("components")),
    _use_dual(getParam<bool>("use_dual"))
{
  scalingFactor(isParamValid("scaling") ? getParam<std::vector<Real>>("scaling")
                                        : std::vector<Real>(_count, 1.));

  if (getParam<bool>("fv") && getParam<bool>("eigen"))
    paramError("eigen", "finite volume (fv=true) variables do not have eigen support");
  if (getParam<bool>("fv") && _fe_type.family != MONOMIAL)
    paramError("family", "finite volume (fv=true) variables must be have MONOMIAL family");
  if (getParam<bool>("fv") && _fe_type.order != 0)
    paramError("order", "finite volume (fv=true) variables currently support CONST order only");

  if (_count > 1)
  {
    auto name0 = _sys.system().variable(_var_num).name();
    std::size_t found = name0.find_last_of("_");
    if (found == std::string::npos)
      mooseError("Error creating ArrayMooseVariable name with base name ", name0);
    _var_name = name0.substr(0, found);
  }
  else
    _var_name = _sys.system().variable(_var_num).name();
}

const std::vector<dof_id_type> &
MooseVariableBase::allDofIndices() const
{
  const auto it = _sys.subproblem()._var_dof_map.find(name());
  if (it != _sys.subproblem()._var_dof_map.end())
    return it->second;
  else
    mooseError("VariableAllDoFMap not prepared for ",
               name(),
               " . Check nonlocal coupling requirement for the variable.");
}

Order
MooseVariableBase::order() const
{
  return _fe_type.order;
}

std::vector<dof_id_type>
MooseVariableBase::componentDofIndices(const std::vector<dof_id_type> & dof_indices,
                                       unsigned int component) const
{
  std::vector<dof_id_type> new_dof_indices(dof_indices);
  if (component != 0)
  {
    if (isNodal())
      for (auto & id : new_dof_indices)
        id += component;
    else
    {
      unsigned int n = dof_indices.size();
      for (auto & id : new_dof_indices)
        id += component * n;
    }
  }
  return new_dof_indices;
}

void
MooseVariableBase::scalingFactor(Real factor)
{
  _scaling_factor.assign(_count, factor);
}

void
MooseVariableBase::scalingFactor(const std::vector<Real> & factor)
{
  _scaling_factor = factor;
}

void
MooseVariableBase::initialSetup()
{
#ifdef MOOSE_GLOBAL_AD_INDEXING
  // Currently the scaling vector is only used through AD residual computing objects
  if (_subproblem.haveADObjects() &&
      (_subproblem.automaticScaling() ||
       (std::find_if(_scaling_factor.begin(), _scaling_factor.end(), [](const Real element) {
          return !MooseUtils::absoluteFuzzyEqual(element, 1.);
        }) != _scaling_factor.end())))
    _sys.addScalingVector();
#endif
}
