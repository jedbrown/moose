//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "MortarSegmentInfo.h"
#include "MooseHashing.h"
#include "ConsoleStreamInterface.h"

// libMesh includes
#include "libmesh/id_types.h"
#include "libmesh/equation_systems.h"
#include "libmesh/elem.h"
#include "libmesh/int_range.h"

// C++ includes
#include <set>
#include <memory>
#include <vector>
#include <unordered_map>

// Forward declarations
namespace libMesh
{
class MeshBase;
class Elem;
}
class GetPot;

// Using statements
using libMesh::boundary_id_type;
using libMesh::dof_id_type;
using libMesh::Elem;
using libMesh::MeshBase;
using libMesh::Node;
using libMesh::Point;
using libMesh::Real;
using libMesh::subdomain_id_type;

typedef boundary_id_type BoundaryID;
typedef subdomain_id_type SubdomainID;

/**
 * This class is a container/interface for the objects involved in
 * automatic generation of mortar spaces.
 */
class AutomaticMortarGeneration : public ConsoleStreamInterface
{
public:
  /**
   * The name of the nodal normals system. We store this in one place
   * so it's easy to change later.
   */
  const static std::string system_name;

  /**
   * Must be constructed with a reference to the Mesh we are
   * generating mortar spaces for.
   */
  AutomaticMortarGeneration(MooseApp & app,
                            MeshBase & mesh_in,
                            const std::pair<BoundaryID, BoundaryID> & boundary_key,
                            const std::pair<SubdomainID, SubdomainID> & subdomain_key,
                            bool on_displaced,
                            bool periodic,
                            const bool debug,
                            const bool correct_edge_dropping);

  /**
   * Once the secondary_requested_boundary_ids and
   * primary_requested_boundary_ids containers have been filled in,
   * call this function to build node-to-Elem maps for the
   * lower-dimensional elements.
   */
  void buildNodeToElemMaps();

  /**
   * Computes and stores the nodal normal vectors in a local data
   * structure instead of using the ExplicitSystem/NumericVector
   * approach. This design was triggered by the way that the
   * GhostingFunctor operates, but I think it is a better/more
   * efficient way to do it anyway.
   */
  void computeNodalNormals();

  /**
   * Since the nodal normals are no longer a variable in the
   * EquationSystems, we need to have an alternate method for writing
   * them out to file for visualization.
   */
  void writeNodalNormalsToFile();

  /**
   * Project secondary nodes (find xi^(2) values) to the closest points on
   * the primary surface.
   * Inputs:
   * - The nodal normals values
   * - mesh
   * - nodes_to_primary_elem_map
   *
   * Outputs:
   * - secondary_node_and_elem_to_xi2_primary_elem
   *
   * Defined in the file project_secondary_nodes.C.
   */
  void projectSecondaryNodes();

  /**
   * (Inverse) project primary nodes to the points on the secondary surface
   * where they would have come from (find (xi^(1) values)).
   *
   * Inputs:
   * - The nodal normals values
   * - mesh
   * - nodes_to_secondary_elem_map
   *
   * Outputs:
   * - primary_node_and_elem_to_xi1_secondary_elem
   *
   * Defined in the file project_primary_nodes.C.
   */
  void projectPrimaryNodes();

  /**
   * Builds the mortar segment mesh once the secondary and primary node
   * projections have been completed.
   *
   * Inputs:
   * - mesh
   * - primary_node_and_elem_to_xi1_secondary_elem
   * - secondary_node_and_elem_to_xi2_primary_elem
   * - nodes_to_primary_elem_map
   *
   * Outputs:
   * - mortar_segment_mesh
   * - msm_elem_to_info
   *
   * Defined in the file build_mortar_segment_mesh.C.
   */
  void buildMortarSegmentMesh();

  /**
   * Builds the mortar segment mesh once the secondary and primary node
   * projections have been completed.
   *
   * Inputs:
   * - mesh
   *
   * Outputs:
   * - mortar_segment_mesh
   * - msm_elem_to_info
   */
  void buildMortarSegmentMesh3d();

  /**
   * Outputs mesh statistics for mortar segment mesh
   */
  void msmStatistics();

  /**
   * Clears the mortar segment mesh and accompanying data structures
   */
  void clear();

  /**
   * returns whether this object is on the displaced mesh
   */
  bool onDisplaced() const { return _on_displaced; }

  /**
   * @return The nodal normals associated with the provided \p secondary_elem
   */
  std::vector<Point> getNodalNormals(const Elem & secondary_elem) const;

  /**
   * Compute the normals at given reference points on a secondary element
   * @param secondary_elem The secondary element used to query for associated nodal normals
   * @param xi1_pts The reference points on the secondary element to evaluate the normals at. The
   * points should only be non-zero in the zeroth entry  because right now our mortar mesh elements
   * are always 1D
   * @return The normals
   */
  std::vector<Point> getNormals(const Elem & secondary_elem,
                                const std::vector<Point> & xi1_pts) const;

  /**
   * Compute the normals at given reference points on a secondary element
   * @param secondary_elem The secondary element used to query for associated nodal normals
   * @param 1d_xi1_pts The reference points on the secondary element to evaluate the normals at. The
   * "points" are single reals corresponding to xi because right now our mortar mesh elements are
   * always 1D
   * @return The normals
   */
  std::vector<Point> getNormals(const Elem & secondary_elem,
                                const std::vector<Real> & oned_xi1_pts) const;

  /**
   * Get list of secondary nodes that don't contribute to interaction with any primary element.
   * Used to enforce zero values on inactive DoFs of nodal variables.
   */
  void computeInactiveLMNodes();

  /**
   * Computes inactive secondary nodes when incorrect edge dropping behavior is enabled
   * (any node touching a partially or fully dropped element is dropped)
   */
  void computeIncorrectEdgeDroppingInactiveLMNodes();

  /**
   * Get list of secondary elems without any corresponding primary elements.
   * Used to enforce zero values on inactive DoFs of elemental variables.
   */
  void computeInactiveLMElems();

  /**
   * @return The mortar interface coupling
   */
  const std::unordered_multimap<dof_id_type, dof_id_type> & mortarInterfaceCoupling() const
  {
    return _mortar_interface_coupling;
  }

  /**
   * @return The primary-secondary boundary ID pairs
   */
  const std::vector<std::pair<boundary_id_type, boundary_id_type>> &
  primarySecondaryBoundaryIDPairs() const
  {
    return _primary_secondary_boundary_id_pairs;
  }

  /**
   * @return The mortar segment mesh
   */
  const MeshBase & mortarSegmentMesh() const { return *_mortar_segment_mesh; }

  /**
   * @return The mortar segment element to corresponding information
   */
  const std::unordered_map<const Elem *, MortarSegmentInfo> & mortarSegmentMeshElemToInfo() const
  {
    return _msm_elem_to_info;
  }

  int dim() const { return _mesh.mesh_dimension(); }

  /**
   * @return The set of nodes on which mortar constraints are not active
   */
  const std::unordered_set<const Node *> & getInactiveLMNodes() const
  {
    return _inactive_local_lm_nodes;
  }

  /**
   * @return The list of secondary elems on which mortar constraint is not active
   */
  const std::unordered_set<const Elem *> & getInactiveLMElems() const
  {
    return _inactive_local_lm_elems;
  }

  bool incorrectEdgeDropping() const { return !_correct_edge_dropping; }

private:
  // Reference to the mesh stored in equation_systems.
  MeshBase & _mesh;

  // The boundary ids corresponding to all the secondary surfaces.
  std::set<boundary_id_type> _secondary_requested_boundary_ids;

  // The boundary ids corresponding to all the primary surfaces.
  std::set<boundary_id_type> _primary_requested_boundary_ids;

  // A list of primary/secondary boundary id pairs corresponding to each
  // side of the mortar interface.
  std::vector<std::pair<boundary_id_type, boundary_id_type>> _primary_secondary_boundary_id_pairs;

  // Map from nodes to connected lower-dimensional elements on the secondary/primary subdomains.
  std::unordered_map<dof_id_type, std::vector<const Elem *>> _nodes_to_secondary_elem_map;
  std::unordered_map<dof_id_type, std::vector<const Elem *>> _nodes_to_primary_elem_map;

  // Similar to the map above, but associates a (Secondary Node, Secondary Elem)
  // pair to a (xi^(2), primary Elem) pair. This allows a single secondary node, which is
  // potentially connected to two elements on the secondary side, to be associated with
  // multiple primary Elem/xi^(2) values to handle the case where the primary and secondary
  // nodes are "matching".
  // In this configuration:
  //
  //    A     B
  // o-----o-----o  (secondary orientation ->)
  //       |
  //       v
  // ------x------ (primary orientation <-)
  //    C     D
  //
  // The entries in the map should be:
  // (Elem A, Node 1) -> (Elem C, xi^(2)=-1)
  // (Elem B, Node 0) -> (Elem D, xi^(2)=+1)
  std::unordered_map<std::pair<const Node *, const Elem *>, std::pair<Real, const Elem *>>
      _secondary_node_and_elem_to_xi2_primary_elem;

  // Same type of container, but for mapping (Primary Node ID, Primary Node,
  // Primary Elem) -> (xi^(1), Secondary Elem) where they are inverse-projected along
  // the nodal normal direction. Note that the first item of the key, the primary
  // node ID, is important for storing the key-value pairs in a consistent order
  // across processes, e.g. this container has to be ordered!
  std::map<std::tuple<dof_id_type, const Node *, const Elem *>, std::pair<Real, const Elem *>>
      _primary_node_and_elem_to_xi1_secondary_elem;

  // 1D Mesh of mortar segment elements which gets built by the call
  // to build_mortar_segment_mesh().
  std::unique_ptr<MeshBase> _mortar_segment_mesh;

  // Map between Elems in the mortar segment mesh and their info
  // structs. This gets filled in by the call to
  // build_mortar_segment_mesh().
  std::unordered_map<const Elem *, MortarSegmentInfo> _msm_elem_to_info;

  // Keeps track of the mapping between lower-dimensional elements and
  // the side_id of the interior_parent which they are.
  std::unordered_map<const Elem *, unsigned int> _lower_elem_to_side_id;

  // A list of primary/secondary subdomain id pairs corresponding to each
  // side of the mortar interface.
  std::vector<std::pair<subdomain_id_type, subdomain_id_type>>
      _primary_secondary_subdomain_id_pairs;

  // The secondary/primary lower-dimensional boundary subdomain ids are the
  // secondary/primary *boundary* ids offset by the value above.
  std::set<subdomain_id_type> _secondary_boundary_subdomain_ids;
  std::set<subdomain_id_type> _primary_boundary_subdomain_ids;

  // Used by the AugmentSparsityOnInterface functor to determine
  // whether a given Elem is coupled to any others across the gap, and
  // to explicitly set up the dependence between interior_parent()
  // elements on the secondary side and their lower-dimensional sides
  // which are on the interface. This latter type of coupling must be
  // explicitly declared when there is no primary_elem for a given
  // mortar segment and you are using e.g.  a P^1-P^0 discretization
  // which does not induce the coupling automatically.
  std::unordered_multimap<dof_id_type, dof_id_type> _mortar_interface_coupling;

  // Container for storing the nodal normal vector associated with each secondary node.
  std::unordered_map<const Node *, Point> _secondary_node_to_nodal_normal;

  // List of inactive lagrange multiplier nodes (for nodal variables)
  std::unordered_set<const Node *> _inactive_local_lm_nodes;

  // List of inactive lagrange multiplier nodes (for elemental variables)
  std::unordered_set<const Elem *> _inactive_local_lm_elems;

  /**
   * Helper function responsible for projecting secondary nodes
   * onto primary elements for a single primary/secondary pair. Called by the class member
   * AutomaticMortarGeneration::project_secondary_nodes().
   */
  void projectSecondaryNodesSinglePair(subdomain_id_type lower_dimensional_primary_subdomain_id,
                                       subdomain_id_type lower_dimensional_secondary_subdomain_id);

  /**
   * Helper function used internally by AutomaticMortarGeneration::project_primary_nodes().
   */
  void projectPrimaryNodesSinglePair(subdomain_id_type lower_dimensional_primary_subdomain_id,
                                     subdomain_id_type lower_dimensional_secondary_subdomain_id);

  /// Whether to print debug output
  const bool _debug;

  /// Whether this object is on the displaced mesh
  const bool _on_displaced;

  /// Whether this object will be generating a mortar segment mesh for periodic constraints
  const bool _periodic;

  /// Whether the mortar segment mesh is distributed
  const bool _distributed;

  /// Newton solve tolerance for node projections
  Real _newton_tolerance = 1e-12;

  /// Tolerance for checking projection xi values. Usually we are checking whether we projected onto
  /// a certain element (in which case -1 <= xi <= 1) or whether we should have *already* projected
  /// a primary node (in which case we error if abs(xi) is sufficiently close to 1)
  Real _xi_tolerance = 1e-6;

  /// Flag to enable regressed treatment of edge dropping where all LM DoFs on edge dropping element
  /// are strongly set to 0.
  bool _correct_edge_dropping;
};
