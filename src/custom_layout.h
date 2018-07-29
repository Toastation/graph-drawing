#pragma once

#include <string>
#include <complex>

#include <tulip/Graph.h>
#include <tulip/TulipPluginHeaders.h>
#include <tulip/BooleanProperty.h>

struct KNode;

/**
 * @brief Tulip plugin implementing a custom static graph drawing algorithm based on the Fast Multipole Method.
 */
class CustomLayout : public tlp::LayoutAlgorithm {
public:
	PLUGININFORMATION("Custom Layout", "Melvin EVEN", "07/2018", "--", "1.0", "Force Directed")
	CustomLayout(const tlp::PluginContext *context);
	~CustomLayout() override;
	bool check(std::string &errorMessage) override;
	bool run() override;

private:
	bool m_cstTemp; // Whether or not the annealing temperature is constant
	bool m_cstInitTemp; // Whether or not the initial annealing temperature is predefined. If false, it is the the initial temperature is sqrt(|V|) 
	bool m_condition; // Whether or not to block certain nodes.
	bool m_multipoleExpansion; // Whether or not to use the multipole extension formula
	bool m_adaptiveCooling; // Whether or not to use the local adaptive cooling strategy
	bool m_stoppingCriterion; // Whether or not to stop the algo earlier if convergence has been detected
	bool m_refinement; // Whether or not to use the refinement strategy.
	bool m_packCC; // Whether or not to pack the connected components after the drawing
	float m_L; // Ideal edge length
	float m_Kr; // Repulsive force constant
	float m_Ks; // Spring force constant
	float m_initTemp; // Initial annealing temperature (if m_cstInitTemp is true)
	float m_initTempFactor; // Factor to apply on the initial annealing temperature (if m_cstInitTemp is false)
	float m_coolingFactor; // Cooling rate of the annealing temperature
	float m_temp; // Global temperature of the graph
	float m_threshold; // The convergence threshold
	float m_maxDisp; // Maximum displament allowed for nodes.
	float m_highEnergyThreshold;
	unsigned int m_iterations; // Number of iterations
	unsigned int m_refinementIterations; // Number of iterations of the refinement process
	unsigned int m_refinementFreq; // Number of iterations in between refinement steps
	unsigned int m_maxPartitionSize; // Maximum number of nodes of the smallest partition of the graph (via KD-tree)
	unsigned int m_pTerm; // Number of term to compute in the p-term multipole expansion
	tlp::BooleanProperty *m_canMove; // Which nodes are able to move during the algorithm
	tlp::BooleanProperty *m_highEnergy; // True if a node has a high energy
	tlp::SizeProperty *m_size; // viewSize
	tlp::DoubleProperty *m_rot;	// viewRotation
	std::vector<tlp::node> m_nodesCopy; // Copy of the graph's nodes, /!\ the order is NOT fixed
	std::vector<tlp::edge> m_removedEdges; // List of removed edges when the graph was made simple
	TLP_HASH_MAP<tlp::node, tlp::Coord> m_disp; // Displacement of each node
	TLP_HASH_MAP<tlp::node, tlp::Coord> m_dispPrev; // Displacement of each during the previous iteration
	TLP_HASH_MAP<tlp::node, tlp::Coord> m_pos; // Current position of each node
	TLP_HASH_MAP<tlp::node, float> m_energy; // Current energy of each node
	
	/**
	 * @brief Prepares the algo (initialises hashmaps, etc) 
	 * @return Returns whether or not the initalisation was successful
	 */
	bool init();

	/**
	 * @brief Main loop of the simulation, computes the drawing and stops after a certain number of iterations or until convergence 
	 * @return The number of iterations done 
	 */
	unsigned int mainLoop(unsigned int maxIterations);

	/**
	 * @brief Computes local temperature for each node 
	 * @param n The node to compute the local temperature from
	 * @return float The local temperature of the node
	 */
	float adaptativeCool(const tlp::node &n);

	/**
	 * @brief Computes the center of the circle circumscribing the set of vertices m_nodesCopy[start...end]
	 * @param start The start index of the set
	 * @param end The end index of the set
	 * @return tlp::Coord The center of the circle circumscribing the set of vertices m_nodesCopy[start...end]
	 */
	tlp::Coord computeCenter(unsigned int start, unsigned int end);

	/**
	 * @brief Computes the radius of the circle circumscribing the set of vertices m_nodesCopy[start...end]
	 * @param start The start index of the set
	 * @param end The end index of the set
	 * @param center The center coordinate of the set of nodes
	 * @return float The radius of the circle circumscribing the set of vertices m_nodesCopy[start...end]
	 */
	float computeRadius(unsigned int start, unsigned int end, tlp::Coord center);

	/**
	 * @brief Auxiliary function of buildKdTree.
	 * @param node The kd-tree node to build
	 * @param level Node's depth in the kd-tree. The depth of the root node is 0.
	 * @param refresh Whether or not to create or refresh the tree.
	 */
	void buildKdTreeAux(KNode *node, unsigned int level, bool refresh);

	/**
	 * @brief Builds a 2d-tree from the plugin's graph. The tree is stored in the graph hierarchy. Wrapper function of buildKdTreeAux.
	 * the root node being the plugin's graph.
	 * Vertices on the even levels of the tree are sorted horizontally, and vertically on the odd levels.
	 * @param refresh Whether or not to create or refresh the tree
	 * @param root If refresh is true, this is the tree that will be refreshed
	 */
	KNode* buildKdTree(bool refresh, KNode *root);

	/**
	 * @brief Computes the coefficients of the multipole expansion of the graph.
	 * @param node The node of the kd-tree on which to compute the coefficients.
	 */
	void computeCoef(KNode *node);

	/**
	 * @brief Computes the repulsives forces that the node is subect to
	 * @param n The node on which to compute the forces
	 * @param kdTree The kd-tree used to approximate the forces
	 * @param computeEnergy If true, computes the node's energy (for the refinement step) 
	 */
	void computeReplForces(const tlp::node &n, KNode *kdTree, bool computeEnergy);

	/**
	 * @brief Refine the drawing : detect high energy nodes and run a simulation allowing only them to move. 
	 * @param averageEnergy 
	 */
	void computeRefinement(double averageEnergy);

	/**
	 * @brief Computes the repulsive force between two nodes
	 * @param dist The distance between nodes
	 * @return float The magnitude of the force
	 */
	float computeReplForce(const tlp::Vec3f &dist) {
		float distNorm = dist.norm();
		if (distNorm == 0) // push the nodes apart slightly 
			return ((float) std::rand()) / (float) RAND_MAX;;
		// return m_Kr / (dist_norm * dist_norm * dist_norm);
		return m_Kr / (distNorm * distNorm);
	}

	/**
	 * @brief Computes the attractive force between two nodes
	 * @param dist The distance between nodes
	 * @return float The magnitude of the force
	 */
	float computeAttrForce(const tlp::Vec3f &dist) {
		float distNorm = dist.norm();
		if (distNorm == 0) // push the nodes apart slightly 
			return ((float) std::rand()) / (float) RAND_MAX;;
		// return m_Ks * (dist_norm - m_L) / dist_norm;
		return m_Ks * distNorm * std::log(distNorm / m_L);
	}

	/**
	 * @brief Computes the integral of the repulsive force formula (i.e the energy associated with the force)
	 * @param dist The distance between nodes
	 * @return float The integral of the repulsive force formula
	 */
	float computeReplForceIntgr(const tlp::Vec3f &dist) {
		return -m_Kr / dist.norm();
	}

	/**
	 * @brief Computes the integral of the attractive force formula (i.e the energy associated with the force)
	 * @param dist The distance between nodes
	 * @return float The integral of the attractive force formula
	 */
	float computeAttrForceIntgr(const tlp::Vec3f &dist) {
		float distNorm = dist.norm();
		return (m_Ks / 9.0f) * (distNorm * distNorm * distNorm * (std::log(distNorm / m_L) - 1) + (m_L * m_L * m_L));
	}
};

/**
 * @brief Node of a kd-tree, stores the necessary information to approximate the repulsive forces
 */
struct KNode {
	unsigned int start; // First index of the sub-list of vertices of CustomLayout::m_nodesCopy
	unsigned int end; // Last index of the sub-list of vertices of CustomLayout::m_nodesCopy
	float radius; // Length between the center of gravity of the vertices and the farthest vertex
	tlp::Coord center; // Center of gravity of the vertices
	float a0; // First coefficient of the multipole expansion
	std::vector<std::complex<float>> coefs; // Coefficents of the p-term sum. 
	KNode *leftChild; // Pointer to the left child
	KNode *rightChild; // Pointer to the right child

	KNode(unsigned int _start=0, unsigned int _end=0, float _radius=0, tlp::Coord _center=tlp::Coord(0)) 
		: start(_start), end(_end), radius(_radius), center(_center) {
		a0 = 0;
		leftChild = nullptr;
		rightChild = nullptr;
	}

	~KNode() {
		
	}

	//*********** DEBUG
	bool isLeaf() {
		return leftChild == nullptr && rightChild == nullptr;
	}

	float depth() {
		if (isLeaf()) 
			return 0;
		return 1 + std::max(leftChild->depth(), rightChild->depth());
	}

	unsigned int size() {
		if (isLeaf())
			return 1;
		unsigned int s = 1;
		if (leftChild != nullptr)
			s += leftChild->size(); 
		if (rightChild != nullptr)
			s += rightChild->size();
		return s;
	}
	
	void print() {
		std::cout << "start: " << start << " | end: " << end << " | span: " << end - start << std::endl;
		std::cout << "center: " << center << " | radius: " << radius << " | depth: " << depth() << std::endl;
		std::cout << "leftChild: " << leftChild << " | rightChild: " << rightChild << std::endl;
		for (unsigned int i = 0; i < 4; ++i) {
			std::cout << "coef" << i << " = " << coefs[i] << std::endl;
		}
		std::cout << "----------------" << std::endl;
		if (leftChild != nullptr)
			leftChild->print();
		if (rightChild != nullptr)
			rightChild->print();
	}

	void printLeaf() {
		if (isLeaf())
			print();
		else {
			if (leftChild != nullptr)
				leftChild->printLeaf();
			if (rightChild != nullptr)
				rightChild->printLeaf();
		}
	}
	//*********** END DEBUG
};

void deleteTree(KNode *tree) {
	if (tree->leftChild != nullptr) {
		deleteTree(tree->leftChild);
		tree->leftChild = nullptr;
	}
	if (tree->rightChild != nullptr) {
		deleteTree(tree->rightChild);
		tree->rightChild = nullptr;
	}
	delete tree;
}