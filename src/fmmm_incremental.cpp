#define _USE_MATH_DEFINES

#include <fmmm_incremental.h>
#include <util.h>

#include <tulip/ForEach.h>
#include <tulip/BooleanProperty.h>
#include <tulip/LayoutProperty.h>
#include <tulip/DoubleProperty.h>
#include <tulip/SizeProperty.h>
#include <tulip/BoundingBox.h>
#include <tulip/DrawingTools.h>

#include <algorithm>
#include <vector>
#include <iterator>
#include <math.h>
#include <cstdlib>
#include <ctime>

#define TAU 2*M_PI

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
        //tlp::Graph *timeline0 = graph->addCloneSubGraph("timeline_0");
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

bool FMMMIncremental::positionNodes(tlp::Graph *g) {
    tlp::BooleanProperty *isNewNode = g->getProperty<tlp::BooleanProperty>("isNewNode");
    tlp::BooleanProperty *adjacentToDeletedEdge = g->getProperty<tlp::BooleanProperty>("adjDeletedEdge");
    tlp::BooleanProperty *canMove = g->getProperty<tlp::BooleanProperty>("canMove");
    tlp::BooleanProperty *positioned = g->getProperty<tlp::BooleanProperty>("positioned");
    tlp::LayoutProperty *pos = g->getProperty<tlp::LayoutProperty>("viewLayout");
    tlp::DoubleProperty *rot = g->getProperty<tlp::DoubleProperty>("viewRotation");
    tlp::SizeProperty *size = g->getProperty<tlp::SizeProperty>("viewSize");
    canMove->setAllNodeValue(false);
    positioned->setAllNodeValue(false);

    tlp::BoundingBox bb = tlp::computeBoundingBox(g, pos, size, rot);
    std::srand(std::time(nullptr));

    tlp::node n;
    forEach(n, g->getNodes()) {
        if (adjacentToDeletedEdge->getNodeValue(n))
            canMove->setNodeValue(n, true);
        if (!isNewNode->getNodeValue(n))
            positioned->setNodeValue(n, true);
    }

    std::vector<tlp::node> positionedNeighbors;
    tlp::node n2;
    forEach(n, isNewNode->getNodesEqualTo(true)) {
        canMove->setNodeValue(n, true);
        
        positionedNeighbors.clear();
        forEach(n2, g->getInOutNodes(n)) {
            if (positioned->getNodeValue(n2))
                positionedNeighbors.push_back(n2);
        }
        unsigned int nbPositionedNeighbors = positionedNeighbors.size();

        if (nbPositionedNeighbors == 0) {
            pos->setNodeValue(n, tlp::Vec3f(std::rand() % int(bb.width()), std::rand() % int(bb.height())));
        } else if (nbPositionedNeighbors == 1) {
            float randomAngle = (float(std::rand()) / float(RAND_MAX)) * float(TAU);
            pos->setNodeValue(n, pos->getNodeValue(positionedNeighbors[0]) + tlp::Vec3f(m_idealEdgeLength * std::cos(randomAngle), m_idealEdgeLength * std::sin(randomAngle)));
            canMove->setNodeValue(positionedNeighbors[0], true);
        } else {
            tlp::Vec3f sumPos;
            for (auto nodeIt = positionedNeighbors.begin(); nodeIt != positionedNeighbors.end(); nodeIt++) {
                sumPos += pos->getNodeValue(*nodeIt);
                canMove->setNodeValue(*nodeIt, true);
            }
            sumPos.setX(sumPos.x() / nbPositionedNeighbors);
            sumPos.setY(sumPos.y() / nbPositionedNeighbors);
            pos->setNodeValue(n, sumPos);
        }

        positioned->setNodeValue(n, true);
    }

    return true;
}

#ifndef FMMMINCREMENTAL_REGISTERED
#define FMMMINCREMENTAL_REGISTERED
PLUGIN(FMMMIncremental)
#endif