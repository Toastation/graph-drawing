#ifndef FMMM_INCREMENTAL_H
#define FMMM_INCREMENTAL_H

#include <string>
#include <tulip/Graph.h>
#include <tulip/TulipPluginHeaders.h>

class FMMMIncremental : public tlp::Algorithm {
public:
    PLUGININFORMATION("FM^3 incremental", "Melvin EVEN", "DATE", "INFO", "RELEASE", "Layout")
    
    FMMMIncremental(const tlp::PluginContext* context) : tlp::Algorithm(context) {

    }

    ~FMMMIncremental() {

    }

    std::string category() const {
        return "Layout";
    }
    
    bool check(std::string &errorMessage);

    bool run();

private:
    float m_idealEdgeLength;

    /**
     * @brief Computes the differences between the 2 newest graphs in the timeline and stores them in the following properties: "isNewNode", "isNewEdge", "adjDeletedEdge"
     * @param oldGraph The previous graph in the timeline
     * @param newGraph The newest graph in the timeline
     * @return true If the computation succeeded
     */
    bool computeDifference(tlp::Graph *oldGraph, tlp::Graph *newGraph);

    /**
     * @brief Positions new nodes, and identifies which nodes should move during the layout process, via the boolean property "canMove"
     * @param g The graph from which to position new nodes
     * @return true If the algo succeeded.
     */
    bool positionNodes(tlp::Graph *g);
};

#endif