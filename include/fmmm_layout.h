#pragma once

#include <string>
#include <tulip/Graph.h>
#include <tulip/TulipPluginHeaders.h>
#include <tulip/BooleanProperty.h>

class FMMMLayoutCustom : public tlp::LayoutAlgorithm {
public:
	PLUGININFORMATION("FM^3 (custom)", "--", "--",
						"--"
						"--",
						"--", 
						"Force Directed")
	FMMMLayoutCustom(const tlp::PluginContext *context);
	~FMMMLayoutCustom() override;
	bool run() override;

private:
	bool m_cstTemp;
	bool m_cstInitTemp;
	float m_L;
	float m_Kr;
	float m_Ks;
	float m_initTemp;
	float m_initTempFactor;
	float m_coolingFactor;
	unsigned int m_iterations;
	unsigned int m_maxPartitionSize;
	tlp::BooleanProperty *m_canMove;

	void build_tree_aux(tlp::Graph *g, tlp::LayoutProperty *pos, tlp::SizeProperty *size, tlp::DoubleProperty *rot, unsigned int level);
	void build_kd_tree();

	/******* FORCE MODEL *******/

	/**
	 * @brief Computes the repulsive force between two nodes
	 * @param dist The distance between nodes
	 * @return float The magnitude of the force
	 */
	float compute_repl_force(const tlp::Vec3f &dist) {
		float dist_norm = dist.norm();
		if (dist_norm == 0) // push the nodes apart slightly 
			return ((float) std::rand()) / (float) RAND_MAX;;
		return m_Kr / (dist_norm * dist_norm * dist_norm);
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
		return m_Ks * (dist_norm - m_L) / dist_norm;
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