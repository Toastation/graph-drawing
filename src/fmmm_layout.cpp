#include <fmmm_layout.h>

#include <tulip/DrawingTools.h>

#include <cstdlib>
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