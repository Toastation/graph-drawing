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
#include <complex>

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
	m_linearMedian = false;
	m_adaptiveCooling = false;

	bool btemp = false;
	int itemp = 0;
	tlp::BooleanProperty *temp;

	if (dataSet != nullptr) {
		if (dataSet->get("iterations", itemp))
			m_iterations = itemp;
		if (dataSet->get("adaptive cooling", btemp))
			m_adaptiveCooling = btemp;
		if (dataSet->get("linear median", btemp))
			m_linearMedian = btemp;
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

		dataSet->get("linear median", m_linearMedian); 		
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

void CustomLayout::init() {
	for (auto n : graph->nodes()) {
		m_disp[n] = tlp::Coord(0, 0, 0);
		m_dispPrev[n] = tlp::Coord(0, 0, 0);
		m_pos[n] = result->getNodeValue(n);
	}
}

bool CustomLayout::run() {
	init();

	const std::vector<tlp::node> &nodes = graph->nodes();
	m_nodesCopy = nodes;
	KNode *kdTree = build_kd_tree(false, nullptr);
	tlp::BoundingBox bb = tlp::computeBoundingBox(graph, result, m_size, m_rot);
	tlp::node n;
	tlp::node u;
	tlp::node v;
	tlp::edge e;
	tlp::Coord dist;
	float t = m_cstInitTemp ? m_initTemp : std::max(std::min(bb.width(), bb.height()) * m_initTempFactor, 2 * m_L);
	float init_temp = t;
	float disp_norm;
	float force;
	bool quit = false;
	unsigned int i = 1;

	std::cout << "Init temp: " << t << std::endl;
	std::string message = "Initial temperature: ";
	message += std::to_string(init_temp);
	pluginProgress->setComment(message);
	auto start = std::chrono::high_resolution_clock::now();
	while (!quit) {
		if (i <= 4 || i % 20 == 0) { 
			build_kd_tree(true, kdTree);
		}

		// forEach (n, graph->getNodes()) { // repulsive forces
		// 	if (!m_condition || m_canMove->getNodeValue(n))
		// 		compute_repl_forces(n, graph);
		// }

		// compute attractive forces TODO: find a way to parallelize
		// for (auto e : graph->edges()) { 
		// 	u = graph->source(e);
		// 	v = graph->target(e);
		// 	tlp::Coord dist = result->getNodeValue(u) - result->getNodeValue(v);
		// 	dist *= compute_attr_force(dist);
		// 	if (!m_condition || m_canMove->getNodeValue(u)) {
		//  		m_disp[u] -= dist;
		// 	}
		// 	if (!m_condition || m_canMove->getNodeValue(v)) {
		// 		m_disp[v] += dist;
		// 	}
		// }

		// // update nodes position
		// #pragma omp parallel for
		// for (unsigned int i = 0; i < graph->numberOfNodes(); i++) { // update positions
		// 	tlp::node n = nodes[i];
		// 	float disp_norm = m_disp[n].norm();
			
		// 	if (disp_norm != 0) {  
		// 		if (m_adaptiveCooling) {
		// 			//m_disp[n] *= std::min(adaptative_cool(n), 200.0f) / disp_norm;
		// 		} else if (!m_adaptiveCooling && t < disp_norm) {
		// 			m_disp[n] *= t / disp_norm;
		// 		}				
		// 	}

		// 	m_pos[n] += m_disp[n];
		// 	m_dispPrev[n] = m_disp[n];
		// 	m_disp[n] = tlp::Coord(0);
		// }

		// // update result property
		// #pragma omp parallel for
		// for (unsigned int i = 0; i < graph->numberOfNodes(); i++) { 
		// 	result->setNodeValue(nodes[i], m_pos[nodes[i]]);
		// }

		// if (!m_adaptiveCooling && !m_cstTemp)
		// 	t *= m_coolingFactor;

		quit = i >= m_iterations || quit;
		++i;

		// pluginProgress->progress(i, m_iterations+2);
		// if (pluginProgress->state() != tlp::TLP_CONTINUE) {
		// 	break;
		// }
	}

	deleteTree(kdTree);

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

tlp::Coord CustomLayout::computeCenter(unsigned int start, unsigned int end) {
	tlp::Coord center(0);
	for (unsigned int i = start; i < end; ++i) {
		center += result->getNodeValue(m_nodesCopy[i]);
	}
	return center / float(end - start);
}

float CustomLayout::computeRadius(unsigned int start, unsigned int end, tlp::Coord center, const tlp::LayoutProperty *layout, const tlp::SizeProperty *size) {
	tlp::Coord result;
	double maxRad = 0;
	for (unsigned int i = start; i < end; ++i) {
		const tlp::Coord &curCoord = layout->getNodeValue(m_nodesCopy[i]);
		tlp::Size curSize(size->getNodeValue(m_nodesCopy[i]) / 2.0f);
		double nodeRad = sqrt(curSize.getW() * curSize.getW() + curSize.getH() * curSize.getH());
		tlp::Coord radDir(curCoord - center);
		double curRad = nodeRad + radDir.norm();

		if (radDir.norm() < 1e-6) {
			curRad = nodeRad;
			radDir = tlp::Coord(1.0, 0.0, 0.0);
		}

		if (curRad > maxRad) {
			maxRad = curRad;
			radDir /= radDir.norm();
			radDir *= curRad;
			result = radDir + center;
		}
	}
	return maxRad;
}

void CustomLayout::build_kd_tree_aux(KNode *node, unsigned int level, bool refresh) {
	unsigned int medianIndex = (node->start + node->end) / 2;
	auto medianIt = m_nodesCopy.begin() + medianIndex;
	auto startIt = m_nodesCopy.begin() + node->start;
	auto endIt = m_nodesCopy.begin() + node->end;
	if (level % 2 == 0) {
		std::nth_element(startIt, medianIt, endIt, [this](tlp::node &a, tlp::node &b) { 
			return result->getNodeValue(a).x() < result->getNodeValue(b).x();
		});
		int medianX = result->getNodeValue(m_nodesCopy[medianIndex]).x();
		std::partition(startIt, endIt, [this, &medianX](tlp::node &a) { 
			return result->getNodeValue(a).x() < medianX;
		});
	} else { 
		std::nth_element(startIt, medianIt, endIt, [this](tlp::node a, tlp::node b) {
			return result->getNodeValue(a).y() < result->getNodeValue(b).y();
		});
		int medianY = result->getNodeValue(m_nodesCopy[medianIndex]).y();
		std::partition(startIt, endIt, [this, &medianY](tlp::node &a) { 
			return result->getNodeValue(a).y() < medianY;
		});
	}
	tlp::Coord leftCenter = computeCenter(node->start, medianIndex);
	tlp::Coord rightCenter = computeCenter(medianIndex, node->end);
	float leftRadius = computeRadius(node->start, medianIndex, leftCenter, result, m_size);
	float rightRadius = computeRadius(medianIndex, node->end, rightCenter, result, m_size);
	if (refresh) {
		node->leftChild->radius = leftRadius;
		node->leftChild->center = leftCenter;
		node->rightChild->radius = rightRadius;
		node->rightChild->center = rightCenter;
	} else {
		node->leftChild = new KNode(node->start, medianIndex, leftRadius, leftCenter);
		node->rightChild = new KNode(medianIndex, node->end, rightRadius, rightCenter);
	}
	if (std::min(medianIndex - node->start, node->end - medianIndex) <= m_maxPartitionSize) return;
	// #pragma omp task
	build_kd_tree_aux(node->leftChild, level + 1, refresh);
	// #pragma omp task	
	build_kd_tree_aux(node->rightChild, level + 1, refresh);
}

KNode* CustomLayout::build_kd_tree(bool refresh, KNode *root=nullptr) {
	if (refresh && root == nullptr) {
		std::cerr << "If refresh is set to true, you must provide a valid root node." << std::endl;
		return root;
	}
	tlp::Coord center = computeCenter(0, m_nodesCopy.size());
	float radius = computeRadius(0, m_nodesCopy.size(), center, result, m_size);
	if (refresh) {
		root->radius = radius;
		root->center = center;
	} else {
		root = new KNode(0, m_nodesCopy.size(), radius, center);
	}
	
	// #pragma omp parallel
	// #pragma omp single
	build_kd_tree_aux(root, 0, refresh);
	return root;
}

// float CustomLayout::adaptative_cool(const tlp::node &n) {
// 	tlp::Vec3f a = m_disp->getNodeValue(n);
// 	tlp::Vec3f b = m_dispPrev->getNodeValue(n);
// 	float a_norm = a.norm();
// 	float b_norm = b.norm();
// 	float angle = std::atan2(a.x() * b.y() - a.y() * b.x(), a.x() * b.x() + a.y() * b.y()); // atan2f(det, dot)
// 	float c;

// 	if (-fPI_6 <= angle && angle <= fPI_6)
// 		c = 2;
// 	else if ((fPI_6 < angle && angle <= f2_PI_6) || (-f2_PI_6 <= angle && angle < -fPI_6))
// 		c = 3.0f / 2;
// 	else if ((f2_PI_6 < angle && angle <= f3_PI_6) || (-f3_PI_6 <= angle && angle < -f2_PI_6))
// 		c = 1;
// 	else if ((f3_PI_6 < angle && angle <= f4_PI_6) || (-f4_PI_6 <= angle && angle < -f3_PI_6))
// 		c = 2.0f / 3;
// 	else
// 		c = 1.0f / 3; 

// 	float res = c * b_norm;
// 	if (a_norm > res && res > 0)
// 		return res;
// 	else 
// 		return a_norm;
// }

// //===================================
// tlp::Graph* inducedSubGraphCustom(std::vector<tlp::node>::iterator begin, std::vector<tlp::node>::iterator end, tlp::Graph *parent, const std::string &name) {
// 	if (parent == nullptr) {
// 		std::cerr << "parent graph must exist to create the induced subgraph" << std::endl;
// 		return nullptr;
// 	}
	
// 	tlp::Graph *result = parent->addSubGraph(name);
	
// 	for (auto it = begin; it != end; ++it) {
// 		result->addNode(*it);
// 	}

// 	tlp::node n;
// 	tlp::edge e;
// 	forEach (n, result->getNodes()) {
// 		forEach (e, parent->getOutEdges(n)) {
// 			if (result->isElement(parent->target(e)))
// 				result->addEdge(e);
// 		}
// 	}
// 	return result;
// }
// //===================================

// void CustomLayout::build_tree_aux(tlp::Graph *g, unsigned int level) {
// 	std::vector<tlp::node> nodes = g->nodes(); // TODO: find a way to not copy the vector each time...
	
// 	int medianIndex = nodes.size() / 2;
// 	auto medianIt = nodes.begin() + medianIndex;

// 	if (level % 2 == 0) {
// 		if (m_linearMedian) { // find the median in linear time then rearrange the nodes around it 
// 			std::nth_element(nodes.begin(), medianIt, nodes.end(), [this](tlp::node &a, tlp::node &b) { 
// 				return result->getNodeValue(a).x() < result->getNodeValue(b).x();
// 			});
// 			int medianX = result->getNodeValue(nodes[medianIndex]).x();
// 			std::partition(nodes.begin(), nodes.end(), [this, &medianX](tlp::node &a) { 
// 				return result->getNodeValue(a).x() < medianX;
// 			});
// 		} else { // sort the nodes in O(n*log(n)) to find the median
// 			std::sort(nodes.begin(), nodes.end(), [this](tlp::node a, tlp::node b) {
// 				return result->getNodeValue(a).x() < result->getNodeValue(b).x();
// 			});
// 		}
// 	} else { // same on the y-axis
// 		if (m_linearMedian) {
// 			std::nth_element(nodes.begin(), nodes.begin() + medianIndex, nodes.end(), [this](tlp::node a, tlp::node b) {
// 				return result->getNodeValue(a).y() < result->getNodeValue(b).y();
// 			});
// 			int medianY = result->getNodeValue(nodes[medianIndex]).y();
// 			std::partition(nodes.begin(), nodes.end(), [this, &medianY](tlp::node &a) { 
// 				return result->getNodeValue(a).y() < medianY;
// 			});
// 		} else {
// 			std::sort(nodes.begin(), nodes.end(), [this](tlp::node a, tlp::node b) {
// 				return result->getNodeValue(a).y() < result->getNodeValue(b).y();
// 			});
// 		}
// 	}

// 	tlp::Graph * leftSubgraph = inducedSubGraphCustom(nodes.begin(), nodes.begin() + medianIndex, g, "unnamed");
// 	tlp::Graph * rightSubgraph = inducedSubGraphCustom(nodes.begin() + medianIndex, nodes.end(), g, "unnamed");
// 	std::pair<tlp::Coord, tlp::Coord> boundingRadiusLeft = tlp::computeBoundingRadius(leftSubgraph, result, m_size, m_rot);
// 	std::pair<tlp::Coord, tlp::Coord> boundingRadiusRight = tlp::computeBoundingRadius(rightSubgraph, result, m_size, m_rot);
// 	leftSubgraph->setAttribute<tlp::Vec3f>("center", boundingRadiusLeft.first);
// 	rightSubgraph->setAttribute<tlp::Vec3f>("center", boundingRadiusRight.first);
// 	leftSubgraph->setAttribute<float>("radius", boundingRadiusLeft.first.dist(boundingRadiusLeft.second));
// 	rightSubgraph->setAttribute<float>("radius", boundingRadiusRight.first.dist(boundingRadiusRight.second));

// 	// // tlp::ColorProperty *leftColor = leftSubgraph->getProperty<tlp::ColorProperty>("viewColor");
// 	// // tlp::ColorProperty *rightColor = leftSubgraph->getProperty<tlp::ColorProperty>("viewColor");
// 	// // tlp::node n;
// 	// // tlp::Color color(std::rand() % 255, std::rand() % 255, std::rand() % 255);
// 	// // forEach (n, leftSubgraph->getNodes()) {
// 	// // 	leftColor->setNodeValue(n, color);
// 	// // }
// 	// // color = tlp::Color(std::rand() % 255, std::rand() % 255, std::rand() % 255);
// 	// // forEach (n, rightSubgraph->getNodes()) {
// 	// // 	rightColor->setNodeValue(n, color);		
// 	// // }

// 	if (m_multipoleExpansion) {
// 		compute_coef(leftSubgraph);
// 		compute_coef(rightSubgraph);
// 	}

// 	if (std::min(leftSubgraph->numberOfNodes(), rightSubgraph->numberOfNodes()) <= m_maxPartitionSize)
// 		return;

// 	build_tree_aux(leftSubgraph, level + 1);
// 	build_tree_aux(rightSubgraph, level + 1);
// }


// void CustomLayout::build_kd_tree() {
// 	if (graph->numberOfNodes() <= m_maxPartitionSize) return;
// 	std::pair<tlp::Coord, tlp::Coord> boundingRadius = tlp::computeBoundingRadius(graph, result, m_size, m_rot);
// 	graph->setAttribute<tlp::Vec3f>("center", boundingRadius.first);
// 	graph->setAttribute<float>("radius", boundingRadius.first.dist(boundingRadius.second));

// 	tlp::Graph *subgraph;
// 	stableForEach (subgraph, graph->getSubGraphs()) { // TODO: check if the stableForEach is necessary
// 		graph->delAllSubGraphs(subgraph);
// 	}

// 	if (m_multipoleExpansion)
// 		compute_coef(graph);
	
// 	build_tree_aux(graph,  0);
// }

// void CustomLayout::compute_coef(tlp::Graph *g) {
// 	tlp::Vec3f *center = static_cast<tlp::Vec3f*>(g->getAttribute("center")->value);	
// 	std::complex<float> z0(center->x(), center->y());
// 	std::complex<float> zi;
// 	std::complex<float> ziMinusz0Overk;
// 	std::vector<std::complex<float>> coefs;
// 	tlp::node v;
// 	tlp::Vec3f pos;
// 	unsigned int nbCoefs = m_pTerm + 1;

// 	coefs.reserve(5);
// 	for (unsigned int i = 0; i < nbCoefs; ++i)
// 		coefs.push_back(std::complex<float>(0, 0));

// 	coefs[0] += g->numberOfNodes(); // a0
	
// 	forEach(v, g->getNodes()) {
// 		pos = result->getNodeValue(v);
// 		zi = std::complex<float>(pos.x(), pos.y());
// 		ziMinusz0Overk = zi - z0; 
// 		for (unsigned int k = 1; k < nbCoefs; ++k) {
// 			coefs[k] = -1.0f * ziMinusz0Overk / (float)k; // ak
// 			ziMinusz0Overk *= ziMinusz0Overk; // next power
// 		}
// 	}
// 	g->setAttribute("coefs", coefs);
// }


// void CustomLayout::compute_repl_forces(const tlp::node &n, tlp::Graph *g) {
// 	tlp::Vec3f dist;
// 	if (g->numberOfNodes() <= m_maxPartitionSize || g->numberOfSubGraphs() == 0) { // leaf node
// 		tlp::node v;
// 		forEach (v, g->getNodes()) {
// 			if (v != n) {
// 				dist = result->getNodeValue(n) - result->getNodeValue(v);
// 				m_disp->setNodeValue(n, m_disp->getNodeValue(n) + dist * compute_repl_force(dist));
// 			}
// 		}
// 		return;
// 	}

// 	tlp::Vec3f *center = static_cast<tlp::Vec3f*>(g->getAttribute("center")->value);
// 	float *radius = static_cast<float*>(g->getAttribute("radius")->value);
	
// 	if (center == nullptr || radius == nullptr) {
// 		std::cerr << "center or radius attributes does not exist" << std::endl;
// 		return;
// 	}

// 	dist = result->getNodeValue(n) - *center;
// 	float dist_norm = dist.norm();
// 	if (dist_norm > *radius) { // internal node, approximation
// 		if (!m_multipoleExpansion) {
// 			m_disp->setNodeValue(n, m_disp->getNodeValue(n) + dist * compute_repl_force(dist) * (float)g->numberOfNodes());
// 		} else {
// 			std::vector<std::complex<float>> *coefs = reinterpret_cast<std::vector<std::complex<float>> *>(g->getAttribute("coefs")->value);
// 			std::complex<float> zMinusz0 = std::complex<float>(result->getNodeValue(n).x(), result->getNodeValue(n).y()) - std::complex<float>(center->x(), center->y());
// 			std::complex<float> potential = (*coefs)[0] / zMinusz0; 
// 			for (unsigned int k = 1; k < m_pTerm + 1; ++k) {
// 				zMinusz0 *= zMinusz0; // next power
// 				potential += (float)k * (*coefs)[k] / zMinusz0;
// 			}
// 			m_disp->setNodeValue(n, m_disp->getNodeValue(n) + (dist / dist_norm) * tlp::Vec3f(potential.real(), -potential.imag()));
// 		}
// 	} else { // internal node, continue
// 		tlp::Graph *sg;
// 		forEach (sg, g->getSubGraphs()) {
// 			compute_repl_forces(n, sg);
// 		}
// 	}
// }