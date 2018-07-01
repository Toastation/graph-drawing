#include <fmmm_layout.h>

#include <tulip/DrawingTools.h>
#include <tulip/ForEach.h>

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

PLUGIN(FMMMLayoutCustom);



FMMMLayoutCustom::FMMMLayoutCustom(const tlp::PluginContext *context) 
	: LayoutAlgorithm(context), m_cstTemp(DEFAUT_CST_TEMP), m_cstInitTemp(DEFAUT_CST_INIT_TEMP), m_L(DEFAULT_L), m_Kr(DEFAULT_KR), m_Ks(DEFAULT_KS),
	  m_initTemp(DEFAULT_INIT_TEMP), m_initTempFactor(DEFAULT_INIT_TEMP_FACTOR), m_coolingFactor(DEFAULT_COOLING_FACTOR), m_iterations(DEFAUT_ITERATIONS), 
	  m_maxPartitionSize(DEFAULT_MAX_PARTITION_SIZE) {
	
}

FMMMLayoutCustom::~FMMMLayoutCustom() {

}

bool FMMMLayoutCustom::run() {
	tlp::LayoutProperty *pos = graph->getLocalProperty<tlp::LayoutProperty>("viewLayout");
	tlp::SizeProperty *size = graph->getLocalProperty<tlp::SizeProperty>("viewSize");
	tlp::DoubleProperty *rot = graph->getLocalProperty<tlp::DoubleProperty>("viewRotation");
	tlp::DoubleProperty *disp = graph->getLocalProperty<tlp::DoubleProperty>("disp");
	tlp::BoundingBox bb = tlp::computeBoundingBox(graph, pos, size, rot);
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

void FMMMLayoutCustom::build_tree_aux(tlp::Graph *g, tlp::LayoutProperty *pos, tlp::SizeProperty *size, tlp::DoubleProperty *rot, unsigned int level) {
	std::vector<tlp::node> nodes = g->nodes();
	if (level % 2 == 0) {
		std::sort(nodes.begin(), nodes.end(), [pos](tlp::node a, tlp::node b) {
			return pos->getNodeValue(a).x() < pos->getNodeValue(b).x();
		});
	} else {
		std::sort(nodes.begin(), nodes.end(), [pos](tlp::node a, tlp::node b) {
			return pos->getNodeValue(a).y() < pos->getNodeValue(b).y();
		});
	}

	unsigned int medianIndex = nodes.size() / 2;
	tlp::Graph * leftSubgraph = inducedSubGraphCustom(nodes.begin(), nodes.begin() + medianIndex, g, "unnamed");
	tlp::Graph * rightSubgraph = inducedSubGraphCustom(nodes.begin() + medianIndex, nodes.end(), g, "unnamed");

	std::pair<tlp::Coord, tlp::Coord> boundingRadiusLeft = tlp::computeBoundingRadius(leftSubgraph, pos, size, rot);
	std::pair<tlp::Coord, tlp::Coord> boundingRadiusRight = tlp::computeBoundingRadius(rightSubgraph, pos, size, rot);
	leftSubgraph->setAttribute("center", boundingRadiusLeft.first);
	rightSubgraph->setAttribute("center", boundingRadiusRight.first);
	leftSubgraph->setAttribute("farthestPoint", boundingRadiusLeft.second);
	rightSubgraph->setAttribute("farthestPoint", boundingRadiusRight.second);
	if (std::max(leftSubgraph->numberOfNodes(), rightSubgraph->numberOfNodes()) <= m_maxPartitionSize)
		return;
	build_tree_aux(leftSubgraph, pos, size, rot, level + 1);
	build_tree_aux(rightSubgraph, pos, size, rot, level + 1);
}


void FMMMLayoutCustom::build_kd_tree() {
	if (graph->numberOfNodes < 4) return;
	tlp::LayoutProperty *pos = graph->getLocalProperty<tlp::LayoutProperty>("viewLayout");
	tlp::SizeProperty *size = graph->getLocalProperty<tlp::SizeProperty>("viewSize");
	tlp::DoubleProperty *rot = graph->getLocalProperty<tlp::DoubleProperty>("viewRotation");
	m_maxPartitionSize = std::sqrt(graph->numberOfNodes); // maximum number of vertices in the smallest partition
	std::pair<tlp::Coord, tlp::Coord> boundingRadius = tlp::computeBoundingRadius(graph, pos, size, rot);
	graph->setAttribute("center", boundingRadius.first);
	graph->setAttribute("farthestPoint", boundingRadius.second);

	tlp::Graph *subgraph;
	forEach (subgraph, graph->getSubGraphs()) {
		graph->delAllSubGraphs(subgraph);
	}

	build_tree_aux(graph, pos, size, rot, 0);
}