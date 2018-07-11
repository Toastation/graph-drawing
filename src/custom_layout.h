#pragma once

#include <string>
#include <tulip/Graph.h>
#include <tulip/TulipPluginHeaders.h>
#include <tulip/BooleanProperty.h>

struct KNode {
	unsigned int start;
	unsigned int end;
	float radius;
	tlp::Coord center;
	KNode *leftChild;
	KNode *rightChild;

	KNode(unsigned int _start=0, unsigned int _end=0, float _radius=0, tlp::Coord _center=tlp::Coord(0)) 
		: start(_start), end(_end), radius(_radius), center(_center) {
		leftChild = nullptr;
		rightChild = nullptr;
	}

	~KNode() {
		
	}

	bool isLeaf() {
		return leftChild == nullptr && rightChild == nullptr;
	}

	float depth() {
		if (isLeaf())
			return 0;
		return 1 + std::max(leftChild->depth(), rightChild->depth());
	}

	void print() {
		std::cout << "start: " << start << " | end: " << end << " | span: " << end - start << std::endl;
		std::cout << "center: " << center << " | radius: " << radius << " | depth: " << depth() << std::endl;
		std::cout << "leftChild: " << leftChild << " | rightChild: " << rightChild << std::endl;
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
	tlp::SizeProperty *m_size;					// viewSize
	tlp::DoubleProperty *m_rot;					// viewRotation
	std::vector<tlp::node> m_nodesCopy;
	TLP_HASH_MAP<tlp::node, tlp::Coord> m_disp;
	TLP_HASH_MAP<tlp::node, tlp::Coord> m_dispPrev;
	TLP_HASH_MAP<tlp::node, tlp::Coord> m_pos;

	void init();

	/**
	 * @brief 
	 * @param n 
	 * @return float 
	 */
	float adaptativeCool(const tlp::node &n);

	/**
	 * @brief Builds the next level of the 2d-tree
	 * @param g The graph to split
	 * @param pos The positions of the nodes (from the root node of the tree)
	 * @param size The sizes of the nodes (from the root node of the tree)
	 * @param rot The rotations of the nodes (from the root node of the tree)
	 * @param level The current depth of the tree
	 */
	void build_tree_aux(tlp::Graph *g, unsigned int level);


	tlp::Coord computeCenter(unsigned int start, unsigned int end);
	float computeRadius(unsigned int start, unsigned int end, tlp::Coord center, const tlp::LayoutProperty *layout, const tlp::SizeProperty *size);
	void buildKdTreeAux(KNode *node, unsigned int level, bool refresh);

	/**
	 * @brief Builds a 2d-tree from the plugin's graph. The tree is stored in the graph hierarchy,
	 * the root node being the plugin's graph.
	 * Vertices on the even levels of the tree are sorted horizontally, and vertically on the odd levels.
	 */
	KNode* buildKdTree(bool refresh, KNode *root);

	/**
	 * @brief Computes the coefficients of the multipole expansion of the graph. Stores the result in
	 * a graph attribute called "coefs" (std::vector<std::complex<float>>)
	 * @param g The graph from which to compute the coefficients
	 */
	void computeCoef(tlp::Graph *g);

	/**
	 * @brief Computes the repulsives forces that the node is subect to
	 * @param n The node on which to compute the forces
	 */
	void computeReplForces(const tlp::node &n, KNode *kdTree);

	/**
	 * @brief Computes the repulsive force between two nodes
	 * @param dist The distance between nodes
	 * @return float The magnitude of the force
	 */
	float computeReplForce(const tlp::Vec3f &dist) {
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
	float computeAttrForce(const tlp::Vec3f &dist) {
		float dist_norm = dist.norm();
		if (dist_norm == 0) // push the nodes apart slightly 
			return ((float) std::rand()) / (float) RAND_MAX;;
		// return m_Ks * (dist_norm - m_L) / dist_norm;
		return m_Ks * dist_norm * std::log(dist_norm / m_L);
	}
};