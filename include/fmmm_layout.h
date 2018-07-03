#pragma once

#include <string>
#include <tulip/Graph.h>
#include <tulip/TulipPluginHeaders.h>
#include <tulip/BooleanProperty.h>

class FMMMLayoutCustom : public tlp::LayoutAlgorithm {
public:
	PLUGININFORMATION("FM^3 (custom)", "Melvin EVEN", "--", "--", "--", "Force Directed")
	FMMMLayoutCustom(const tlp::PluginContext *context);
	~FMMMLayoutCustom() override;
	bool check(std::string &errorMessage) override;
	bool run() override;

private:
	bool m_cstTemp;
	bool m_cstInitTemp;
	bool m_condition;
	float m_L;
	float m_Kr;
	float m_Ks;
	float m_initTemp;
	float m_initTempFactor;
	float m_coolingFactor;
	unsigned int m_iterations;
	unsigned int m_maxPartitionSize;
	tlp::BooleanProperty *m_canMove;
	tlp::LayoutProperty *m_disp;
	tlp::SizeProperty *m_size;
	tlp::DoubleProperty *m_rot;

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

	void compute_coef(tlp::Graph *g);

	/**
	 * @brief Computes the repulsives forces that the node is subect to
	 * @param n The node on which to compute the forces
	 */
	void compute_repl_forces(tlp::node n, tlp::Graph *g);

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
		return 512*8 / (dist_norm * dist_norm * dist_norm);
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
		return dist_norm * std::log(dist_norm / 0.704*8);
	}

	/**
	 * @brief Computes the integral of the repulsive force between two nodes
	 * @param dist The distance between nodes
	 * @return float The integral of the repulsive force between two nodes
	 */
	float repl_force_integral(const tlp::Vec3f &dist) {
		return -(m_Kr / dist.norm());
	}

	/**
	 * @brief Computes the integral of the attractive force between two nodes
	 * @param dist The distance between nodes
	 * @return float The integral of the repulsive force between two nodes
	 */
	float attr_force_integral(const tlp::Vec3f &dist) {
		float dist_norm = dist.norm();
		return m_Ks * (((dist_norm * dist_norm) / 2) - (m_L * dist_norm));
	}
};