#pragma once

#include <string>
#include <tulip/Graph.h>
#include <tulip/TulipPluginHeaders.h>

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
	float m_L;
	float m_Kr;
	float m_Ks;
	float m_initTemp;
	float m_coolingFactor;
	float m_maxPartitionSize;
};