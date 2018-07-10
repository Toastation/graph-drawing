#pragma once

#include <string>
#include <tulip/Graph.h>
#include <tulip/TulipPluginHeaders.h>
#include <tulip/BooleanProperty.h>

#define forEachCustom(A, B)                                                                              \
  for (tlp::_TLP_IT<decltype(A)> _it_foreach = B; tlp::_tlp_if_test(A, _it_foreach);)

class CustomLayout : public tlp::LayoutAlgorithm {
public:
	PLUGININFORMATION("Custom Layout", "Melvin EVEN", "07/2018", "--", "1.0", "Force Directed")
	CustomLayout(const tlp::PluginContext *context);
	~CustomLayout() override;
	bool check(std::string &errorMessage) override;
	bool run() override;

private:
	bool m_cstTemp;								// Whether or not the annealing temperature is constant
	bool m_cstInitTemp;							// Whether or not the initial annealing temperature is predefined. If false, it is the the initial temperature is sqrt(|V|) 
	bool m_condition;							// Whether or not to block certain nodes.
	bool m_multipoleExpansion;					// Whether or not to use the multipole extension formula
	bool m_linearMedian;						// If true, finds the median in linear time with introselect. If false, uses quicksort to find the median.
	bool m_adaptiveCooling;				
	float m_L;									// Ideal edge length
	float m_Kr;									// Repulsive force constant
	float m_Ks;									// Spring force constant
	float m_initTemp;							// Initial annealing temperature (if m_cstInitTemp is true)
	float m_initTempFactor;						// Factor to apply on the initial annealing temperature (if m_cstInitTemp is false)
	float m_coolingFactor;						// Cooling rate of the annealing temperature
	unsigned int m_iterations;					// Number of iterations
	unsigned int m_maxPartitionSize;			// Maximum number of nodes of the smallest partition of the graph (via KD-tree)
	unsigned int m_pTerm;						// Number of term to compute in the p-term multipole expansion
	tlp::BooleanProperty *m_canMove;			// Which nodes are able to move
	tlp::LayoutProperty *m_disp;				// Displacement of each node
	tlp::LayoutProperty *m_dispPrev;			// Displacement of each node during the previous iteration
	tlp::SizeProperty *m_size;					// viewSize
	tlp::DoubleProperty *m_rot;					// viewRotation

	/**
	 * @brief 
	 * @param n 
	 * @return float 
	 */
	float adaptative_cool(const tlp::node &n);

	/**
	 * @brief Builds the next level of the 2d-tree
	 * @param g The graph to split
	 * @param pos The positions of the nodes (from the root node of the tree)
	 * @param size The sizes of the nodes (from the root node of the tree)
	 * @param rot The rotations of the nodes (from the root node of the tree)
	 * @param level The current depth of the tree
	 */
	void build_tree_aux(tlp::Graph *g, unsigned int level);
	
	/**
	 * @brief Builds a 2d-tree from the plugin's graph. The tree is stored in the graph hierarchy,
	 * the root node being the plugin's graph.
	 * Vertices on the even levels of the tree are sorted horizontally, and vertically on the odd levels.
	 */
	void build_kd_tree();

	/**
	 * @brief Computes the coefficients of the multipole expansion of the graph. Stores the result in
	 * a graph attribute called "coefs" (std::vector<std::complex<float>>)
	 * @param g The graph from which to compute the coefficients
	 */
	void compute_coef(tlp::Graph *g);

	/**
	 * @brief Computes the repulsives forces that the node is subect to
	 * @param n The node on which to compute the forces
	 */
	void compute_repl_forces(const tlp::node &n, tlp::Graph *g);

	/**
	 * @brief Computes the repulsive force between two nodes
	 * @param dist The distance between nodes
	 * @return float The magnitude of the force
	 */
	float compute_repl_force(const tlp::Vec3f &dist) {
		float dist_norm = dist.norm();
		if (dist_norm == 0) // push the nodes apart slightly 
			return ((float) std::rand()) / (float) RAND_MAX;;
		// return m_Kr / (dist_norm * dist_norm * dist_norm);
		return m_Kr / (dist_norm * dist_norm);
	}

	/**
	 * @brief Computes the attractive force between two nodes
	 * @param dist The distance between nodes
	 * @return float The magnitude of the force
	 */
	float compute_attr_force(const tlp::Vec3f &dist) {
		float dist_norm = dist.norm();
		if (dist_norm == 0) // push the nodes apart slightly 
			return ((float) std::rand()) / (float) RAND_MAX;;
		// return m_Ks * (dist_norm - m_L) / dist_norm;
		return m_Ks * dist_norm * std::log(dist_norm / m_L);
	}
};