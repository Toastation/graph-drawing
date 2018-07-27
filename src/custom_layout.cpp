#define _USE_MATH_DEFINES

#include "custom_layout.h"

#include <tulip/BoundingBox.h>
#include <tulip/DrawingTools.h>
#include <tulip/ForEach.h>
#include <tulip/DataSet.h>
#include <tulip/LayoutProperty.h>
#include <tulip/DoubleProperty.h>	
#include <tulip/SizeProperty.h>
#include <tulip/BooleanProperty.h>	
#include <tulip/ColorProperty.h>

#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <chrono>

const float DEFAULT_L = 10.0f;
const float DEFAULT_KR = 100.0f;
const float DEFAULT_KS = 1.0f;
const float DEFAULT_INIT_TEMP = 200.0f;
const float DEFAULT_INIT_TEMP_FACTOR = 0.2f;
const float DEFAULT_COOLING_FACTOR = 0.95f;
const float DEFAULT_THRESHOLD = 0.1f;
const float DEFAULT_MAX_DISP = 200.0f;
const float DEFAULT_HIGH_ENERGY_THRESHOlD = 1.0f;
const float MULTIPOLE_EXPANSION_FACTOR = 100.0f;
const unsigned int DEFAULT_REFINEMENT_ITERATIONS = 20;
const unsigned int DEFAULT_REFINEMENT_FREQ = 30;
const unsigned int DEFAULT_MAX_PARTITION_SIZE = 4;
const unsigned int DEFAULT_PTERM = 4;
const unsigned int DEFAUT_ITERATIONS = 300;

const float fPI_6 = M_PI / 6.0f;
const float f2_PI_6 = 2.0f * fPI_6;
const float f3_PI_6 = 3.0f * fPI_6;
const float f4_PI_6 = 4.0f * fPI_6;

PLUGIN(CustomLayout)

CustomLayout::CustomLayout(const tlp::PluginContext *context) 
	: LayoutAlgorithm(context), m_L(DEFAULT_L), m_Kr(DEFAULT_KR), m_Ks(DEFAULT_KS),
	  m_initTemp(DEFAULT_INIT_TEMP), m_initTempFactor(DEFAULT_INIT_TEMP_FACTOR), m_coolingFactor(DEFAULT_COOLING_FACTOR), m_threshold(DEFAULT_THRESHOLD), m_maxDisp(DEFAULT_MAX_DISP), 
	  m_highEnergyThreshold(DEFAULT_HIGH_ENERGY_THRESHOlD), m_iterations(DEFAUT_ITERATIONS), m_refinementIterations(DEFAULT_REFINEMENT_ITERATIONS), m_refinementFreq(DEFAULT_REFINEMENT_FREQ),
	  m_maxPartitionSize(DEFAULT_MAX_PARTITION_SIZE), m_pTerm(DEFAULT_PTERM) {
	addInParameter<bool>("adaptive cooling", "If true, the algo uses a local cooling function based on the angle between each node's movement. Else it uses a global linear cooling function.", "", false);
	addInParameter<bool>("stopping criterion", "If true, stops the algo before the maximum number of iterations if the graph has converged. See \"convergence threshold\"", "", false);
	addInParameter<bool>("multipole expansion", "If true, apply a 4-term multipole expansion for more accurate layout. May affect performances.", "", false);
	addInParameter<bool>("block nodes", "If true, only nodes in the set \"movable nodes\" will move.", "", false);
	addInParameter<bool>("refinement", "", "", false);	
	addInParameter<bool>("pack connected components", "", "true", false);	
	addInParameter<unsigned int>("max iterations", "The maximum number of iterations of the algorithm.", "300", false);
	addInParameter<unsigned int>("max displacement", "The maximum length a node can move. Very high values or very low values may result in chaotic behavior.", "200", false);
	addInParameter<unsigned int>("refinement iterations", "", "20", false);
	addInParameter<unsigned int>("refinement frequency", "", "30", false);	
	addInParameter<float>("ideal edge length", "The ideal edge length.", "10", false);
	addInParameter<float>("spring force strength", "Factor of the spring force", "1", false);
	addInParameter<float>("repulsive force strength", "Factor of the repulsive force", "100", false);
	addInParameter<float>("convergence threshold", "If the average node energy is lower than this threshold, the graph is considered to have converged and the algorithm stops. Only taken into consideration if \"stopping criterion\" is true", "0.1", false);
	addInParameter<float>("high energy threshold", "Threshold above which a node is consired to have a high energy", "1.0", false);	
	addInParameter<tlp::BooleanProperty>("movable nodes", "Set of nodes allowed to move. Only taken into account if \"blocked nodes\" is true", "", false);
	addDependency("Connected Component Packing (Polyomino)", "1.0");
	m_cstTemp = false;
	m_cstInitTemp = false;
	m_condition = false;
	m_multipoleExpansion = false;
	m_adaptiveCooling = false;
	m_stoppingCriterion = false;
	m_refinement = false;
}

CustomLayout::~CustomLayout() {

}

bool CustomLayout::check(std::string &errorMessage) {
	// TODO: check if the graph is simple
	return true;
}

bool CustomLayout::run() {
	if (!init())
		return false;

	std::cout << "Initial temperature: " << m_temp << std::endl;
	auto start = std::chrono::high_resolution_clock::now();

	unsigned int it = mainLoop(m_iterations);
	
	// update result property
	#pragma omp parallel for
	for (unsigned int i = 0; i < graph->numberOfNodes(); i++) { 
		result->setNodeValue(m_nodesCopy[i], m_pos[m_nodesCopy[i]]);
	}

	if (m_packCC && !tlp::ConnectedTest::isConnected(graph)) { // pack connected components
		std::string errorMessage;
		tlp::DataSet ds;
		ds.set("coordinates", result);
		ds.set("node size", m_size);
		ds.set("rotation", m_rot);
		ds.set("margin", 50);
		graph->applyPropertyAlgorithm("Connected Component Packing (Polyomino)", result, errorMessage, pluginProgress, &ds);
	}

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = end - start;
	std::cout << "elapsed: " << elapsed.count() << std::endl;
	std::cout << "Iterations done: " << it  << std::endl;

	return true;
}

bool CustomLayout::init() {
	bool btemp = false;
	int itemp = 0;
	float ftemp = 0.0f;
	tlp::BooleanProperty *temp;

	// receive user's data
	if (dataSet != nullptr) {
		if (dataSet->get("max iterations", itemp))
			m_iterations = itemp;
		if (dataSet->get("refinement iterations", itemp))
			m_refinementIterations = itemp;
		if (dataSet->get("refinement frequency", itemp))
			m_refinementFreq = itemp;
		if (dataSet->get("max displacement", ftemp))
			m_maxDisp = itemp;
		if (dataSet->get("ideal edge length", ftemp))
			m_L = ftemp;
		if (dataSet->get("spring force strength", ftemp))
			m_Ks = ftemp;
		if (dataSet->get("repulsive force strength", ftemp))
			m_Kr = ftemp;
		if (dataSet->get("convergence threshold", ftemp))
			m_threshold = ftemp;
		if (dataSet->get("high energy threshold", ftemp))
			m_highEnergyThreshold = ftemp;
		if (dataSet->get("adaptive cooling", btemp))
			m_adaptiveCooling = btemp;
		if (dataSet->get("stopping criterion", btemp))
			m_stoppingCriterion = btemp;
		if (dataSet->get("multipole expansion", btemp))
			m_multipoleExpansion = btemp;
		if (dataSet->get("block nodes", btemp))
			m_condition = btemp;
		if (dataSet->get("refinement", btemp))
			m_refinement = btemp;
		if (dataSet->get("movable nodes", temp))
			m_canMove = temp;
		if (dataSet->get("pack connected components", btemp))
			m_packCC = btemp;
		else if (m_condition) {
			pluginProgress->setError("\"block nodes\" parameter is true but no BooleanProperty was given. Check parameter \"movable nodes\"");
			return false;
		}
	}
	
	// initialise hashmaps and temperature
	result->copy(graph->getProperty<tlp::LayoutProperty>("viewLayout"));
	result->setAllEdgeValue(std::vector<tlp::Vec3f>(0));

	m_size = graph->getLocalProperty<tlp::SizeProperty>("viewSize");
	m_rot = graph->getLocalProperty<tlp::DoubleProperty>("viewRotation");
	m_highEnergy = graph->getLocalProperty<tlp::BooleanProperty>("highEnergy");

	tlp::BoundingBox bb = tlp::computeBoundingBox(graph, result, m_size, m_rot);
	m_temp = m_cstInitTemp ? m_initTemp : std::max(std::min(bb.width(), bb.height()) * m_initTempFactor, 2 * m_L);

	m_nodesCopy = graph->nodes();
	for (auto n : m_nodesCopy) {
		m_energy[n] = 0;
		m_disp[n] = tlp::Coord(0, 0, 0);
		m_dispPrev[n] = tlp::Coord(0, 0, 0);
		m_pos[n] = result->getNodeValue(n);
	}
	return true;
}

unsigned int CustomLayout::mainLoop(unsigned int maxIterations) {
	KNode *kdTree = buildKdTree(false, nullptr);
	bool quit = false;
	bool refinement = false;
	unsigned int it = 1;
	float averageDisp = 0;
	float averageEnergy = 0;

	while (!quit) {
		if (it <= 4 || it % 10 == 0)
			buildKdTree(true, kdTree); // refresh the kd-tree

		refinement = m_condition && m_refinement && it > 0 && it % m_refinementFreq == 0; // no need to refine if there are no blocked nodes...

		// compute repulsive forces
		#pragma omp parallel for
		for (unsigned int i = 0; i < m_nodesCopy.size(); ++i) {
			if (!m_condition || m_canMove->getNodeValue(m_nodesCopy[i]))
				computeReplForces(m_nodesCopy[i], kdTree, refinement);
		}

		//compute attractive forces TODO: find a way to parallelize
		for (auto e : graph->edges()) { 
			const tlp::node &u = graph->source(e);
			const tlp::node &v = graph->target(e);
			tlp::Coord dist = m_pos[u] - m_pos[v];
			dist *= computeAttrForce(dist);
			if (!m_condition || m_canMove->getNodeValue(u)) {
		 		m_disp[u] -= dist;
				if (refinement)
					m_energy[u] += computeAttrForceIntgr(dist);
			}
			if (!m_condition || m_canMove->getNodeValue(v)) {
				m_disp[v] += dist;
				if (refinement)
					m_energy[v] += computeAttrForceIntgr(dist);				
			}
		}

		// update nodes position
		#pragma omp parallel for
		for (unsigned int i = 0; i < m_nodesCopy.size(); i++) {
			const tlp::node &n = m_nodesCopy[i];
			float dispNorm = m_disp[n].norm();
			float cooledNorm = dispNorm;
			if (dispNorm != 0) {  
				if (m_adaptiveCooling) {
					cooledNorm = std::min(adaptativeCool(n), m_maxDisp);
					m_disp[n] *=  cooledNorm / dispNorm;
				} else if (!m_adaptiveCooling && m_temp < dispNorm) {
					cooledNorm = m_temp;
					m_disp[n] *= cooledNorm / dispNorm;
				}				
			}
			if (refinement)
				averageEnergy += m_energy[n];
			averageDisp += cooledNorm;
			m_pos[n] += m_disp[n];
			m_dispPrev[n] = m_disp[n];
			m_disp[n] = tlp::Coord(0);
		}

		// detect convergence
		if (m_stoppingCriterion && averageDisp / m_nodesCopy.size() <= m_threshold)
			quit = true;
		averageDisp = 0;

		if (refinement || (quit && m_refinement))
			computeRefinement(averageEnergy);

		if (!m_adaptiveCooling && !m_cstTemp)
			m_temp *= m_coolingFactor;

		quit = it > maxIterations || quit;
		++it;

		// pluginProgress->progress(it, maxIterations + 1);
		// if (pluginProgress->state() != tlp::TLP_CONTINUE) {
		// 	break;
		// }
	}
	deleteTree(kdTree);
	return it;
}

float CustomLayout::adaptativeCool(const tlp::node &n) {
	tlp::Vec3f a = m_disp[n];
	tlp::Vec3f b = m_dispPrev[n];
	float a_norm = a.norm();
	float b_norm = b.norm();
	float angle = std::atan2(a.x() * b.y() - a.y() * b.x(), a.x() * b.x() + a.y() * b.y()); // atan2(det, dot)
	float scalar;

	// assign a scalar to b based on the angle between a and b
	if (-fPI_6 <= angle && angle <= fPI_6)
		scalar = 2;
	else if ((fPI_6 < angle && angle <= f2_PI_6) || (-f2_PI_6 <= angle && angle < -fPI_6))
		scalar = 3.0f / 2;
	else if ((f2_PI_6 < angle && angle <= f3_PI_6) || (-f3_PI_6 <= angle && angle < -f2_PI_6))
		scalar = 1;
	else if ((f3_PI_6 < angle && angle <= f4_PI_6) || (-f4_PI_6 <= angle && angle < -f3_PI_6))
		scalar = 2.0f / 3;
	else
		scalar = 1.0f / 3; 
	
	float res = scalar * b_norm;
	if (a_norm > res && res > 0)
		return res;
	else 
		return a_norm;
}

tlp::Coord CustomLayout::computeCenter(unsigned int start, unsigned int end) {
	tlp::Coord center(0);
	for (unsigned int i = start; i < end; ++i) {
		center += m_pos[m_nodesCopy[i]];
	}
	return center / float(end - start);
}

float CustomLayout::computeRadius(unsigned int start, unsigned int end, tlp::Coord center) {
	double maxRad = 0;
	for (unsigned int i = start; i < end; ++i) {
		const tlp::Coord &curCoord = m_pos[m_nodesCopy[i]];
		tlp::Size curSize(m_size->getNodeValue(m_nodesCopy[i]) / 2.0f);
		double nodeRad = sqrt(curSize.getW() * curSize.getW() + curSize.getH() * curSize.getH());
		tlp::Coord radDir(curCoord - center);
		double curRad = nodeRad + radDir.norm();
		if (radDir.norm() < 1e-6) {
			curRad = nodeRad;
			radDir = tlp::Coord(1.0, 0.0, 0.0);
		}
		if (curRad > maxRad)
			maxRad = curRad;
	}
	return maxRad;
}

void CustomLayout::buildKdTreeAux(KNode *node, unsigned int level, bool refresh) {
	unsigned int medianIndex = (node->start + node->end) / 2;
	auto medianIt = m_nodesCopy.begin() + medianIndex;
	auto startIt = m_nodesCopy.begin() + node->start;
	auto endIt = m_nodesCopy.begin() + node->end;

	// find the median and rearrange m_nodesCopy around it
	if (level % 2 == 0) {
		std::nth_element(startIt, medianIt, endIt, [this](tlp::node &a, tlp::node &b) { 
			return m_pos[a].x() < m_pos[b].x();
		});
		int medianX = m_pos[m_nodesCopy[medianIndex]].x();
		std::partition(startIt, endIt, [this, &medianX](tlp::node &a) { 
			return m_pos[a].x() < medianX;
		});
	} else { 
		std::nth_element(startIt, medianIt, endIt, [this](tlp::node a, tlp::node b) {
			return m_pos[a].y() < m_pos[b].y();
		});
		int medianY = m_pos[m_nodesCopy[medianIndex]].y();
		std::partition(startIt, endIt, [this, &medianY](tlp::node &a) { 
			return m_pos[a].y() < medianY;
		});
	}

	// compute the new center and radius
	tlp::Coord leftCenter = computeCenter(node->start, medianIndex);
	tlp::Coord rightCenter = computeCenter(medianIndex, node->end);
	float leftRadius = computeRadius(node->start, medianIndex, leftCenter);
	float rightRadius = computeRadius(medianIndex, node->end, rightCenter);
	if (refresh) {
		node->leftChild->radius = leftRadius;
		node->leftChild->center = leftCenter;
		node->rightChild->radius = rightRadius;
		node->rightChild->center = rightCenter;
	} else {
		node->leftChild = new KNode(node->start, medianIndex, leftRadius, leftCenter);
		node->rightChild = new KNode(medianIndex, node->end, rightRadius, rightCenter);
	}

	// compute the multipolar expansion coefficients
	if (m_multipoleExpansion) {
		computeCoef(node->leftChild);
		computeCoef(node->rightChild);
	}

	if (std::min(medianIndex - node->start, node->end - medianIndex) <= m_maxPartitionSize) return;
	#pragma omp task
	buildKdTreeAux(node->leftChild, level + 1, refresh);
	#pragma omp task	
	buildKdTreeAux(node->rightChild, level + 1, refresh);
}

KNode* CustomLayout::buildKdTree(bool refresh, KNode *root=nullptr) {
	if (refresh && root == nullptr) {
		pluginProgress->setError("Refresh is true but root node is null in CustomLayout::buildKdTree");
		return root;
	}
	
	// compute the center, radius and start the recursion 
	tlp::Coord center = computeCenter(0, m_nodesCopy.size());
	float radius = computeRadius(0, m_nodesCopy.size(), center);
	if (refresh) {
		root->radius = radius;
		root->center = center;
	} else {
		root = new KNode(0, m_nodesCopy.size(), radius, center);
	}

	// compute the multipolar expansion coefficients
	if (m_multipoleExpansion)
		computeCoef(root);
	
	#pragma omp parallel
	#pragma omp single
	buildKdTreeAux(root, 0, refresh);
	return root;
}

void CustomLayout::computeCoef(KNode *node) {
	std::complex<float> ziMinusz0Overk;
	std::vector<std::complex<float>> coefs;
	unsigned int nbCoefs = m_pTerm;
	coefs.reserve(nbCoefs);
	for (unsigned int i = 0; i < nbCoefs; ++i)
		coefs.push_back(std::complex<float>(0, 0));
	node->a0 = node->end - node->start;
	for (unsigned int i = node->start; i < node->end; ++i) {
		tlp::node v = m_nodesCopy[i];
		tlp::Coord dist = m_pos[v] - node->center;
		ziMinusz0Overk = std::complex<float>(dist.x(), dist.y()); 
		for (unsigned int k = 1; k < nbCoefs+1; ++k) {
			coefs[k-1] += -1.0f * ziMinusz0Overk / (float)k; // ak
			ziMinusz0Overk *= ziMinusz0Overk; // next power
		}
	}
	node->coefs = coefs;
}

void CustomLayout::computeReplForces(const tlp::node &n, KNode *kdTree, bool computeEnergy) {
	if (kdTree == nullptr) {
		pluginProgress->setError("nullptr kdTree in CustomLayout::computeReplForces");
		return;
	}
	tlp::Coord dist = m_pos[n] - kdTree->center;
	float distNorm = dist.norm();

	// leaf node -> compute the extact repulsive forces
	if (kdTree->leftChild == nullptr && kdTree->rightChild == nullptr) {
		for (unsigned int i = kdTree->start; i < kdTree->end; ++i) {
			const tlp::node &v = m_nodesCopy[i];
			if (n != v) {
				tlp::Coord dist = m_pos[n] - m_pos[v];
				dist *= computeReplForce(dist);
				m_disp[n] += dist;
				if (computeEnergy)
					m_energy[n] += computeReplForceIntgr(dist);
			}
		}
		return;
	}

	// internal node -> approximate the forces if outside of the bounds, else continue the recursion 
	if (distNorm > kdTree->radius) {	
		if (!m_multipoleExpansion) {
			dist *= (kdTree->end - kdTree->start) * computeReplForce(dist);
			m_disp[n] += dist;
		} else {
			std::complex<float> zMinusz0 = std::complex<float>(dist.x(), dist.y());
			std::complex<float> potential = kdTree->a0 / zMinusz0; 
			for (unsigned int k = 1; k < m_pTerm+1; ++k) {
				zMinusz0 *= zMinusz0; // next power
				potential += (float)k * kdTree->coefs[k-1] / zMinusz0;
			}
			m_disp[n] += tlp::Coord(potential.real(), -potential.imag()) * MULTIPOLE_EXPANSION_FACTOR;
		}
		if (computeEnergy) 
			m_energy[n] += computeReplForceIntgr(dist);
	}	else { 
		computeReplForces(n, kdTree->leftChild, computeEnergy);
		computeReplForces(n, kdTree->rightChild, computeEnergy);
	}
}

void CustomLayout::computeRefinement(double totalEnergy) {
	totalEnergy /= m_nodesCopy.size();
	for (unsigned int i = 0; i < m_nodesCopy.size(); ++i) {
		m_highEnergy->setNodeValue(m_nodesCopy[i], ((m_energy[m_nodesCopy[i]] - totalEnergy) / totalEnergy) > m_highEnergyThreshold);
		m_energy[m_nodesCopy[i]] = 0;
	}	
	tlp::BooleanProperty *canMoveTemp = m_canMove;
	bool refinementTemp = m_refinement;
	bool stoppingCriterionTemp = m_stoppingCriterion;
	bool conditionTemp = m_condition;
	m_stoppingCriterion = false;
	m_refinement = false;
	m_canMove = m_highEnergy;
	m_condition = true;
	mainLoop(m_refinementIterations);
	m_refinement = refinementTemp;
	m_canMove = canMoveTemp;
	m_stoppingCriterion = stoppingCriterionTemp;
	m_condition = conditionTemp;
}