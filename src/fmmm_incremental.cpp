#include <fmmm_incremental.h>
#include <util.h>

#include <tulip/ForEach.h>

#include <algorithm>
#include <vector>
#include <iterator>

bool FMMMIncremental::check(std::string &errorMessage) {
    return true;
}

bool FMMMIncremental::run() {
    // std::vector<tlp::Graph *> subgraphs;
    // tlp::Graph * g;
    // forEach (g, graph->getSubGraphs()) {
    //     subgraphs.push_back(g);
    // }
    //std::sort(subgraphs.begin(), subgraphs.end(), [](tlp::Graph * a, tlp::Graph * b) { return a->getId() < b->getId(); }); // TODO: figure out what's the deal wuth getId()
    
    // we should check the order of the subgraphs, right now it's the responsability of the user

    int nb_subgraphs = graph->numberOfSubGraphs();
    if (nb_subgraphs == 0) {
        tlp::Graph *timeline0 = graph->addCloneSubGraph("timeline_0");
        // run static layout algo
    } else if (nb_subgraphs == 1) {
        // run static layout algo
    } else {
        tlp::Graph *newGraph = graph->getNthSubGraph(nb_subgraphs - 1);
        tlp::Graph *oldGraph = graph->getNthSubGraph(nb_subgraphs - 2);
        computeDifference(oldGraph, newGraph);
        // run incremental layout algo
    }

    return true;
}

bool FMMMIncremental::computeDifference(tlp::Graph *oldGraph, tlp::Graph *newGraph) {
    tlp::BooleanProperty *isNewNode = newGraph->getProperty<tlp::BooleanProperty>("isNewNode");
    tlp::BooleanProperty *isNewEdge = newGraph->getProperty<tlp::BooleanProperty>("isNewEdge");
    tlp::BooleanProperty *adjacentToDeletedEdge = newGraph->getProperty<tlp::BooleanProperty>("adjDeletedEdge");
    isNewNode->setAllNodeValue(false);
    isNewEdge->setAllNodeValue(false);
    adjacentToDeletedEdge->setAllNodeValue(false);

    tlp::node n;
    forEach(n, newGraph->getNodes()) {
        if (!isInIterator<tlp::node>(oldGraph->getNodes(), n)) {
            isNewNode->setNodeValue(n, true);
        }
    }

    tlp::edge e;
    forEach(e, newGraph->getEdges()) {
        if (!isInIterator<tlp::edge>(oldGraph->getEdges(), e)) {
            isNewEdge->setEdgeValue(e, true);
        }
    }

    tlp::node source;
    tlp::node target;
    forEach(e, oldGraph->getEdges()) {
        if (!isInIterator<tlp::edge>(newGraph->getEdges(), e)) {
            isNewEdge->setEdgeValue(e, true);
            source = graph->source(e);
            target = graph->target(e);
            if (!isInIterator<tlp::node>(newGraph->getNodes(), source)) 
                adjacentToDeletedEdge->setNodeValue(source, true);
            if (!isInIterator<tlp::node>(newGraph->getNodes(), target))
                adjacentToDeletedEdge->setNodeValue(target, true);
        }
    }
    return true;    
}

#ifndef FMMMINCREMENTAL_REGISTERED
#define FMMMINCREMENTAL_REGISTERED
PLUGIN(FMMMIncremental)
#endif