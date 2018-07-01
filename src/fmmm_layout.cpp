#include <fmmm_layout.h>

#include <tulip/DrawingTools.h>
#include <tulip/ForEach.h>
#include <tulip/DataSet.h>
#include <tulip/LayoutProperty.h>
#include <tulip/DoubleProperty.h>	
#include <tulip/SizeProperty.h>
#include <tulip/BooleanProperty.h>	

#include <cstdlib>
#include <cmath>
#include <algorithm>


const bool DEFAUT_CST_TEMP = false;
const bool DEFAUT_CST_INIT_TEMP = false;
const float DEFAULT_L = 10.0f;
const float DEFAULT_KR = 6250.0f;
const float DEFAULT_KS = 1.0f;
const float DEFAULT_INIT_TEMP = 200.0f;
const float DEFAULT_INIT_TEMP_FACTOR = 0.2f;
const float DEFAULT_COOLING_FACTOR = 0.95f;
const unsigned int DEFAULT_MAX_PARTITION_SIZE = 20;
const unsigned int DEFAUT_ITERATIONS = 300;

PLUGIN(FMMMLayoutCustom)

FMMMLayoutCustom::FMMMLayoutCustom(const tlp::PluginContext *context) 
	: LayoutAlgorithm(context), m_cstTemp(DEFAUT_CST_TEMP), m_cstInitTemp(DEFAUT_CST_INIT_TEMP), m_L(DEFAULT_L), m_Kr(DEFAULT_KR), m_Ks(DEFAULT_KS),
	  m_initTemp(DEFAULT_INIT_TEMP), m_initTempFactor(DEFAULT_INIT_TEMP_FACTOR), m_coolingFactor(DEFAULT_COOLING_FACTOR), m_iterations(DEFAUT_ITERATIONS), 
	  m_maxPartitionSize(DEFAULT_MAX_PARTITION_SIZE) {
	addInParameter<tlp::BooleanProperty>("movable nodes", "Set of nodes allowed to move. If nothing is given, all nodes are allowed to move.", "", false);
}

FMMMLayoutCustom::~FMMMLayoutCustom() {

}

bool FMMMLayoutCustom::check(std::string &errorMessage) {
	m_condition = false;

	if (dataSet != nullptr)
		m_condition = dataSet->get("movable nodes", m_canMove);

	result->copy(graph->getProperty<tlp::LayoutProperty>("viewLayout"));
	m_disp = graph->getLocalProperty<tlp::LayoutProperty>("disp");
	m_size = graph->getLocalProperty<tlp::SizeProperty>("viewSize");
	m_rot = graph->getLocalProperty<tlp::DoubleProperty>("viewRotation");
	return true;
}


bool FMMMLayoutCustom::run() {
	std::cout << "running..." << std::endl;
	tlp::BoundingBox bb = tlp::computeBoundingBox(graph, result, m_size, m_rot);
	float t = m_cstInitTemp ? m_initTemp : std::min(bb.width(), bb.height()) * m_initTempFactor;

	std::cout << "Init temp: " << t << std::endl;

	bool quit = false;
	unsigned int i = 1;

	tlp::node n;
	tlp::node u;
	tlp::node v;
	tlp::edge e;
	tlp::Vec3f dist;
	float force;
	float disp_norm;
	while (!quit) {
		if (i <= 4 || i % 20 == 0) {
			build_kd_tree();
		}

		forEach (n, graph->getNodes()) {
			if (!m_condition || m_canMove->getNodeValue(n))
				compute_repl_forces(n, graph);
		}

		forEach (e, graph->getEdges()) {
			u = graph->source(e);
			v = graph->target(e);
			dist = result->getNodeValue(u) - result->getNodeValue(v);
			force = compute_attr_force(dist);
			if (!m_condition || m_canMove->getNodeValue(u))
				m_disp->setNodeValue(u, m_disp->getNodeValue(u) - dist * force);
			if (!m_condition || m_canMove->getNodeValue(v))
				m_disp->setNodeValue(v, m_disp->getNodeValue(v) + dist * force);
		}

		forEach (n, graph->getNodes()) {
			disp_norm = m_disp->getNodeValue(n).norm();
			if (disp_norm != 0 && t < disp_norm) 
				m_disp->setNodeValue(n, (m_disp->getNodeValue(n) / disp_norm) * t);
			result->setNodeValue(n, result->getNodeValue(n) + m_disp->getNodeValue(n));
			m_disp->setNodeValue(n, tlp::Vec3f());
		}		

		if (!m_cstTemp)
			t *= m_coolingFactor;

		quit = i > m_iterations || quit;
		++i;
	}

	std::cout << "Iterations done: " << i << std::endl;

	return true;
}

//===================================
tlp::Graph* inducedSubGraphCustom(std::vector<tlp::node>::iterator begin, std::vector<tlp::node>::iterator end, tlp::Graph *parent, const std::string &name) {
	if (parent == nullptr) {
		std::cerr << "parent graph must exist to create the induced subgraph" << std::endl;
		return nullptr;
	}

	tlp::Graph *result = parent->addSubGraph(name);
	for (auto it = begin; it != end; ++it) {
		result->addNode(*it);
	}

	tlp::node n;
	tlp::edge e;
	forEach (n, result->getNodes()) {
		forEach (e, parent->getOutEdges(n)) {
			if (result->isElement(parent->target(e)))
				result->addEdge(e);
		}
	}
	return result;
}
//===================================

void FMMMLayoutCustom::build_tree_aux(tlp::Graph *g, unsigned int level) {
	std::vector<tlp::node> nodes = g->nodes(); // TODO: find a way to not copy the vector each time...
	if (level % 2 == 0) {
		std::sort(nodes.begin(), nodes.end(), [this](tlp::node a, tlp::node b) {
			return result->getNodeValue(a).x() < result->getNodeValue(b).x();
		});
	} else {
		std::sort(nodes.begin(), nodes.end(), [this](tlp::node a, tlp::node b) {
			return result->getNodeValue(a).y() < result->getNodeValue(b).y();
		});
	}

	unsigned int medianIndex = nodes.size() / 2;
	tlp::Graph * leftSubgraph = inducedSubGraphCustom(nodes.begin(), nodes.begin() + medianIndex, g, "unnamed");
	tlp::Graph * rightSubgraph = inducedSubGraphCustom(nodes.begin() + medianIndex, nodes.end(), g, "unnamed");

	std::pair<tlp::Coord, tlp::Coord> boundingRadiusLeft = tlp::computeBoundingRadius(leftSubgraph, result, m_size, m_rot);
	std::pair<tlp::Coord, tlp::Coord> boundingRadiusRight = tlp::computeBoundingRadius(rightSubgraph, result, m_size, m_rot);
	leftSubgraph->setAttribute<tlp::Vec3f>("center", boundingRadiusLeft.first);
	rightSubgraph->setAttribute<tlp::Vec3f>("center", boundingRadiusRight.first);
	leftSubgraph->setAttribute<float>("radius", boundingRadiusLeft.first.dist(boundingRadiusLeft.second));
	rightSubgraph->setAttribute<float>("radius", boundingRadiusRight.first.dist(boundingRadiusRight.second));
	if (std::max(leftSubgraph->numberOfNodes(), rightSubgraph->numberOfNodes()) <= m_maxPartitionSize)
		return;
	build_tree_aux(leftSubgraph, level + 1);
	build_tree_aux(rightSubgraph, level + 1);
}


void FMMMLayoutCustom::build_kd_tree() {
	if (graph->numberOfNodes() < 4) return;
	m_maxPartitionSize = std::sqrt(graph->numberOfNodes()); // maximum number of vertices in the smallest partition
	std::pair<tlp::Coord, tlp::Coord> boundingRadius = tlp::computeBoundingRadius(graph, result, m_size, m_rot);
	graph->setAttribute<tlp::Vec3f>("center", boundingRadius.first);
	graph->setAttribute<float>("radius", boundingRadius.first.dist(boundingRadius.second));

	tlp::Graph *subgraph;
	stableForEach (subgraph, graph->getSubGraphs()) { // TODO: check if the stableForEach is necessary
		graph->delAllSubGraphs(subgraph);
	}

	build_tree_aux(graph,  0);
}

void FMMMLayoutCustom::compute_repl_forces(tlp::node n, tlp::Graph *g) {
	tlp::Vec3f *center = static_cast<tlp::Vec3f*>(g->getAttribute("center")->value);
	float *radius = static_cast<float*>(g->getAttribute("radius")->value);
	tlp::Vec3f dist = result->getNodeValue(n) - *center;
	if (dist.norm() > *radius) {
		m_disp->setNodeValue(n, m_disp->getNodeValue(n) + dist * compute_repl_force(dist) * (float)g->numberOfNodes());
	} else {
		if (g->numberOfSubGraphs() == 0) { // leaf case 
			tlp::node v;
			forEach (v, g->getNodes()) {
				if (v != n) {
					dist = result->getNodeValue(n) - result->getNodeValue(v);
					m_disp->setNodeValue(n, m_disp->getNodeValue(n) + dist * compute_repl_force(dist));
				}
			}
		} else {
			tlp::Graph *sg;
			forEach (sg, g->getSubGraphs()) {
				compute_repl_forces(n, sg);
			}
		}
	}
}