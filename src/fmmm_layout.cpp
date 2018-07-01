#include <fmmm_layout.h>

#include <tulip/DrawingTools.h>
#include <tulip/ForEach.h>
#include <tulip/DataSet.h>

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
	m_pos = graph->getLocalProperty<tlp::LayoutProperty>("viewLayout");
	m_disp = graph->getLocalProperty<tlp::LayoutProperty>("disp");
	m_size = graph->getLocalProperty<tlp::SizeProperty>("viewSize");
	m_rot = graph->getLocalProperty<tlp::DoubleProperty>("viewRotation");
}

FMMMLayoutCustom::~FMMMLayoutCustom() {

}

bool FMMMLayoutCustom::run() {
	tlp::BoundingBox bb = tlp::computeBoundingBox(graph, m_pos, m_size, m_rot);
	float t = m_cstInitTemp ? m_initTemp : std::min(bb.width(), bb.height()) * m_initTempFactor;

	std::cout << "Init temp: " << t << std::endl;

	bool quit = false;
	unsigned int i = 1;

	while (!quit) {
		if (i <= 4 || i % 20 == 0) {
			// TODO: rebuild kd-tree
		}

		// TODO compute repl forces

		// TODO compute attr forces

		// TODO update pos

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
	if (parent == nullptr)
		std::cerr << "parent graph must exist to create the induced subgraph" << std::endl;
		return nullptr;

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
			return m_pos->getNodeValue(a).x() < m_pos->getNodeValue(b).x();
		});
	} else {
		std::sort(nodes.begin(), nodes.end(), [this](tlp::node a, tlp::node b) {
			return m_pos->getNodeValue(a).y() < m_pos->getNodeValue(b).y();
		});
	}

	unsigned int medianIndex = nodes.size() / 2;
	tlp::Graph * leftSubgraph = inducedSubGraphCustom(nodes.begin(), nodes.begin() + medianIndex, g, "unnamed");
	tlp::Graph * rightSubgraph = inducedSubGraphCustom(nodes.begin() + medianIndex, nodes.end(), g, "unnamed");

	std::pair<tlp::Coord, tlp::Coord> boundingRadiusLeft = tlp::computeBoundingRadius(leftSubgraph, m_pos, m_size, m_rot);
	std::pair<tlp::Coord, tlp::Coord> boundingRadiusRight = tlp::computeBoundingRadius(rightSubgraph, m_pos, m_size, m_rot);
	leftSubgraph->setAttribute<tlp::Vec3f>("center", boundingRadiusLeft.first);
	rightSubgraph->setAttribute<tlp::Vec3f>("center", boundingRadiusRight.first);
	leftSubgraph->setAttribute<tlp::Vec3f>("radius", boundingRadiusLeft.first.dist(boundingRadiusLeft.second));
	rightSubgraph->setAttribute<tlp::Vec3f>("radius", boundingRadiusRight.first.dist(boundingRadiusRight.second));
	if (std::max(leftSubgraph->numberOfNodes(), rightSubgraph->numberOfNodes()) <= m_maxPartitionSize)
		return;
	build_tree_aux(leftSubgraph, level + 1);
	build_tree_aux(rightSubgraph, level + 1);
}


void FMMMLayoutCustom::build_kd_tree() {
	if (graph->numberOfNodes() < 4) return;
	m_maxPartitionSize = std::sqrt(graph->numberOfNodes()); // maximum number of vertices in the smallest partition
	std::pair<tlp::Coord, tlp::Coord> boundingRadius = tlp::computeBoundingRadius(graph, m_pos, m_size, m_rot);
	graph->setAttribute<tlp::Vec3f>("center", boundingRadius.first);
	graph->setAttribute<tlp::Vec3f>("radius", boundingRadius.first.dist(boundingRadius.second));

	tlp::Graph *subgraph;
	forEach (subgraph, graph->getSubGraphs()) {
		graph->delAllSubGraphs(subgraph);
	}

	build_tree_aux(graph,  0);
}

void FMMMLayoutCustom::compute_repl_forces(tlp::node n, tlp::Graph *g) {
	tlp::Vec3f *center = static_cast<tlp::Vec3f*>(g->getAttribute("center")->value);
	float *radius = static_cast<float*>(g->getAttribute("radius")->value);
	tlp::Vec3f dist = m_pos->getNodeValue(n) - *center;
	if (dist.norm() > *radius) {
		m_disp->setNodeValue(n, m_disp->getNodeValue(n) + dist * compute_repl_force(dist) * (float)g->numberOfNodes());
	} else {
		if (g->numberOfSubGraphs() == 0) { // leaf case 
			tlp::node v;
			forEach (v, g->getNodes()) {
				if (v != n) {
					dist = m_pos->getNodeValue(n) - m_pos->getNodeValue(v);
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