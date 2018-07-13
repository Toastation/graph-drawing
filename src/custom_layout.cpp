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

const bool DEFAUT_CST_TEMP = false;
const bool DEFAUT_CST_INIT_TEMP = false;
const float DEFAULT_L = 10.0f;
const float DEFAULT_KR = 100.0f;
const float DEFAULT_KS = 1.0f;
const float DEFAULT_INIT_TEMP = 200.0f;
const float DEFAULT_INIT_TEMP_FACTOR = 0.2f;
const float DEFAULT_COOLING_FACTOR = 0.95f;
const unsigned int DEFAULT_MAX_PARTITION_SIZE = 4;
const unsigned int DEFAULT_PTERM = 4;
const unsigned int DEFAUT_ITERATIONS = 300;

const float fPI_6 = M_PI / 6.0f;
const float f2_PI_6 = 2.0f * fPI_6;
const float f3_PI_6 = 3.0f * fPI_6;
const float f4_PI_6 = 4.0f * fPI_6;

PLUGIN(CustomLayout)

CustomLayout::CustomLayout(const tlp::PluginContext *context) 
	: LayoutAlgorithm(context), m_cstTemp(DEFAUT_CST_TEMP), m_cstInitTemp(DEFAUT_CST_INIT_TEMP), m_L(DEFAULT_L), m_Kr(DEFAULT_KR), m_Ks(DEFAULT_KS),
	  m_initTemp(DEFAULT_INIT_TEMP), m_initTempFactor(DEFAULT_INIT_TEMP_FACTOR), m_coolingFactor(DEFAULT_COOLING_FACTOR), m_iterations(DEFAUT_ITERATIONS), 
	  m_maxPartitionSize(DEFAULT_MAX_PARTITION_SIZE), m_pTerm(DEFAULT_PTERM) {
	addInParameter<unsigned int>("iterations", "", "300", false);
	addInParameter<bool>("adaptive cooling", "", "", false);
	addInParameter<bool>("linear median", "", "true", false);
	addInParameter<bool>("multipole expansion", "", "", false);
	addInParameter<bool>("block nodes", "If true, only nodes in the set \"movable nodes\" will move.", "", false);
	addInParameter<tlp::BooleanProperty>("movable nodes", "Set of nodes allowed to move. Only taken into account if \"blocked nodes\" is true", "", false);
	addDependency("Connected Component Packing", "1.0");
}

CustomLayout::~CustomLayout() {

}

bool CustomLayout::check(std::string &errorMessage) {
	m_condition = false;
	m_multipoleExpansion = false;
	m_adaptiveCooling = false;

	bool btemp = false;
	int itemp = 0;
	tlp::BooleanProperty *temp;

	if (dataSet != nullptr) {
		if (dataSet->get("iterations", itemp))
			m_iterations = itemp;
		if (dataSet->get("adaptive cooling", btemp))
			m_adaptiveCooling = btemp;
		if (dataSet->get("multipole expansion", btemp))
			m_multipoleExpansion = btemp;
		if (dataSet->get("block nodes", btemp))
			m_condition = btemp;
		if (dataSet->get("movable nodes", temp))
			m_canMove = temp;
		else if (m_condition) {
			pluginProgress->setError("\"block nodes\" parameter is true but no BooleanProperty was given. Check parameter \"movable nodes\"");
			return false;
		}

		dataSet->get("multipole expansion", m_multipoleExpansion); 		
		dataSet->get("block nodes", m_condition); 		
		dataSet->get("movable nodes", m_canMove);
	}
	
	result->copy(graph->getProperty<tlp::LayoutProperty>("viewLayout"));
	m_size = graph->getLocalProperty<tlp::SizeProperty>("viewSize");
	m_rot = graph->getLocalProperty<tlp::DoubleProperty>("viewRotation");
	
	result->setAllEdgeValue(std::vector<tlp::Vec3f>(0));

	return true;
}

bool CustomLayout::run() {
	init();

	KNode *kdTree = buildKdTree(false, nullptr);
	bool quit = false;
	unsigned int i = 1;

	std::cout << "Initial temperature: " << m_temp << std::endl;
	std::string message = "Initial temperature: ";
	message += std::to_string(m_temp);
	pluginProgress->setComment(message);
	auto start = std::chrono::high_resolution_clock::now();

	while (!quit) {
		if (i <= 4 || i % 20 == 0) { 
			buildKdTree(true, kdTree);
		}

		for (auto n : m_nodesCopy) {
			if (!m_condition || m_canMove->getNodeValue(n))
				computeReplForces(n, kdTree);
		}

		//compute attractive forces TODO: find a way to parallelize
		for (auto e : graph->edges()) { 
			const tlp::node &u = graph->source(e);
			const tlp::node &v = graph->target(e);
			tlp::Coord dist = m_pos[u] - m_pos[v];
			dist *= computeAttrForce(dist);
			if (!m_condition || m_canMove->getNodeValue(u)) {
		 		m_disp[u] -= dist;
			}
			if (!m_condition || m_canMove->getNodeValue(v)) {
				m_disp[v] += dist;
			}
		}

		// update nodes position
		#pragma omp parallel for
		for (unsigned int i = 0; i < m_nodesCopy.size(); i++) { // update positions
			const tlp::node &n = m_nodesCopy[i];
			float disp_norm = m_disp[n].norm();
			
			if (disp_norm != 0) {  
				if (m_adaptiveCooling) {
					m_disp[n] *= std::min(adaptativeCool(n), 200.0f) / disp_norm;
				} else if (!m_adaptiveCooling && m_temp < disp_norm) {
					m_disp[n] *= m_temp / disp_norm;
				}				
			}

			m_pos[n] += m_disp[n];
			m_dispPrev[n] = m_disp[n];
			m_disp[n] = tlp::Coord(0);
		}

		if (!m_adaptiveCooling && !m_cstTemp)
			m_temp *= m_coolingFactor;

		quit = i >= m_iterations || quit;
		++i;

		// pluginProgress->progress(i, m_iterations+2);
		// if (pluginProgress->state() != tlp::TLP_CONTINUE) {
		// 	break;
		// }
	}

	deleteTree(kdTree);

	// update result property
	#pragma omp parallel for
	for (unsigned int i = 0; i < graph->numberOfNodes(); i++) { 
		result->setNodeValue(m_nodesCopy[i], m_pos[m_nodesCopy[i]]);
	}

	if (!tlp::ConnectedTest::isConnected(graph)) { // pack connected components
		std::string errorMessage;
		tlp::LayoutProperty tmpLayout(graph);
		tlp::DataSet ds;
		ds.set("coordinates", result);
		graph->applyPropertyAlgorithm("Connected Component Packing", &tmpLayout, errorMessage, pluginProgress, &ds);
		*result = tmpLayout;
		return true;
	}

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = end - start;
	std::cout << "elapsed: " << elapsed.count() << std::endl;
	std::cout << "Iterations done: " << i  << std::endl;

	return true;
}

void CustomLayout::init() {
	m_nodesCopy = graph->nodes();
	
	tlp::BoundingBox bb = tlp::computeBoundingBox(graph, result, m_size, m_rot);
	m_temp = m_cstInitTemp ? m_initTemp : std::max(std::min(bb.width(), bb.height()) * m_initTempFactor, 2 * m_L);

	for (auto n : m_nodesCopy) {
		m_disp[n] = tlp::Coord(0, 0, 0);
		m_dispPrev[n] = tlp::Coord(0, 0, 0);
		m_pos[n] = result->getNodeValue(n);
	}
}

float CustomLayout::adaptativeCool(const tlp::node &n) {
	tlp::Vec3f a = m_disp[n];
	tlp::Vec3f b = m_dispPrev[n];
	float a_norm = a.norm();
	float b_norm = b.norm();
	float angle = std::atan2(a.x() * b.y() - a.y() * b.x(), a.x() * b.x() + a.y() * b.y()); // atan2(det, dot)
	float c;

	if (-fPI_6 <= angle && angle <= fPI_6)
		c = 2;
	else if ((fPI_6 < angle && angle <= f2_PI_6) || (-f2_PI_6 <= angle && angle < -fPI_6))
		c = 3.0f / 2;
	else if ((f2_PI_6 < angle && angle <= f3_PI_6) || (-f3_PI_6 <= angle && angle < -f2_PI_6))
		c = 1;
	else if ((f3_PI_6 < angle && angle <= f4_PI_6) || (-f4_PI_6 <= angle && angle < -f3_PI_6))
		c = 2.0f / 3;
	else
		c = 1.0f / 3; 

	float res = c * b_norm;
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
	
	// tlp::ColorProperty *color = graph->getProperty<tlp::ColorProperty>("viewColor");
	// tlp::node n;
	// tlp::Color c(std::rand() % 255, std::rand() % 255, std::rand() % 255);
	// for (unsigned int i = node->leftChild->start; i < node->leftChild->end; ++i) {
	// 	color->setNodeValue(m_nodesCopy[i], c);
	// }	
	// c = tlp::Color(std::rand() % 255, std::rand() % 255, std::rand() % 255);
	// for (unsigned int i = node->rightChild->start; i < node->rightChild->end; ++i) {
	// 	color->setNodeValue(m_nodesCopy[i], c);
	// }

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
	
	tlp::Coord center = computeCenter(0, m_nodesCopy.size());
	float radius = computeRadius(0, m_nodesCopy.size(), center);
	if (refresh) {
		root->radius = radius;
		root->center = center;
	} else {
		root = new KNode(0, m_nodesCopy.size(), radius, center);
	}

	if (m_multipoleExpansion)
		computeCoef(root);
	
	#pragma omp parallel
	#pragma omp single
	buildKdTreeAux(root, 0, refresh);
	return root;
}

void CustomLayout::computeReplForces(const tlp::node &n, KNode *kdTree) {
	if (kdTree == nullptr) {
		pluginProgress->setError("nullptr kdTree in CustomLayout::computeReplForces");
		return;
	}

	// leaf node case -> compute the exact repulsive forces between n and the vertices contained in the leaf node
	if (kdTree->leftChild == nullptr && kdTree->rightChild == nullptr) {
		for (unsigned int i = kdTree->start; i < kdTree->end; ++i) {
			const tlp::node &v = m_nodesCopy[i];
			if (n != v) {
				tlp::Coord dist = m_pos[n] - m_pos[v];
				dist *= computeReplForce(dist);
				m_disp[n] += dist;
			}
		}
		return;
	}

	tlp::Coord dist = m_pos[n] - kdTree->center;
	float distNorm = dist.norm();
	
	// internal node case and n is outside the partition -> compute the approximate repulsive force between n and the vertices contained by the kd-tree node 
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
			m_disp[n] += tlp::Coord(potential.real(), -potential.imag())*100.0f;
		}
	}
	// internal node case and n is inside the partition -> continue through the kd-tree 
	else { 
		computeReplForces(n, kdTree->leftChild);
		computeReplForces(n, kdTree->rightChild);
	}
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