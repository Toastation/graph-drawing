#define _USE_MATH_DEFINES

#include "incremental.h"

#include <tulip/ForEach.h>
#include <tulip/BooleanProperty.h>
#include <tulip/LayoutProperty.h>
#include <tulip/DoubleProperty.h>
#include <tulip/SizeProperty.h>
#include <tulip/ColorProperty.h>
#include <tulip/BoundingBox.h>
#include <tulip/DrawingTools.h>
#include <tulip/Iterator.h>

#include <algorithm>
#include <vector>
#include <iterator>
#include <math.h>
#include <cstdlib>
#include <ctime>

const float TAU = 2.0f * M_PI;
const float DEFAULT_IDEAL_EDGE_LENGTH = 20.0f;
const tlp::Color DEFAULT_NEW_COLOR = tlp::Color(18, 173, 42);
const tlp::Color DEFAULT_ADJ_TO_DELETED_COLOR = tlp::Color(180, 10, 0);

/**
 * @brief Returns true if the element is in the iterator.
 * @param it The iterator to search through.
 * @param elem The element to search for.
 * @return true if the element is in the iterator.
 */
template<class T>
bool isInIterator(tlp::Iterator<T> *it, T elem) {
    if (!it) {
        std::cerr << "Invalid argument \'it\' in isIn()" << std::endl;
        return false;
    }
    while (it->hasNext()) {
        if (elem == it->next()) {
            return true;
        }
    }
    return false;
} 

FMMMIncremental::FMMMIncremental(const tlp::PluginContext* context) 
    : tlp::Algorithm(context), m_idealEdgeLength(DEFAULT_IDEAL_EDGE_LENGTH), m_newColor(DEFAULT_NEW_COLOR), m_adjToDeletedColor(DEFAULT_ADJ_TO_DELETED_COLOR) {
    addDependency("Custom Layout", "1.0");
}

bool FMMMIncremental::check(std::string &errorMessage) {
    std::srand(std::time(NULL));
    return true;
}

bool FMMMIncremental::run() {
    if (graph->numberOfSubGraphs() == 0)
        pluginProgress->setError("There is no timeline!");

    std::vector<tlp::Graph *> subgraphs;
    std::string errorMessage;
    tlp::Graph *g;
    forEach (g, graph->getSubGraphs()) {
        subgraphs.push_back(g);
    }
    // we should check the order of the subgraphs, right now it's the responsability of the user

    tlp::ColorProperty *globalColors = graph->getProperty<tlp::ColorProperty>("viewColor");     
    tlp::LayoutProperty *previousPos = graph->getProperty<tlp::LayoutProperty>("viewLayout");
    tlp::LayoutProperty *currentPos;
    tlp::ColorProperty *currentColors;
    std::string message;
    for (unsigned int i = 0; i < subgraphs.size(); ++i) {
        message = "Computing timeline... " + i;
        message += "/ " + subgraphs.size();
        pluginProgress->setComment(message);
        currentPos = subgraphs[i]->getLocalProperty<tlp::LayoutProperty>("viewLayout"); // overwriting global property "viewLayout", it is now empty however
        currentColors = subgraphs[i]->getLocalProperty<tlp::ColorProperty>("viewColor");     
        currentPos->copy(previousPos);
        currentColors->copy(globalColors);
        tlp::DataSet ds;
        if (i > 0) { // no need to block nodes and compute differences for the first graph of the timeline
            computeDifference(subgraphs[i-1], subgraphs[i]);
            positionNodes(subgraphs[i]);
            ds.set("block nodes", true);
            ds.set("movable nodes", subgraphs[i]->getLocalProperty<tlp::BooleanProperty>("canMove"));
        }
        subgraphs[i]->applyPropertyAlgorithm("Custom Layout", currentPos, errorMessage, nullptr, &ds);
        previousPos = currentPos;
        pluginProgress->progress(i, subgraphs.size());
    }
    return true;
}

bool FMMMIncremental::computeDifference(tlp::Graph *oldGraph, tlp::Graph *newGraph) {
    tlp::BooleanProperty *isNewNode = newGraph->getLocalProperty<tlp::BooleanProperty>("isNewNode");
    tlp::BooleanProperty *isNewEdge = newGraph->getLocalProperty<tlp::BooleanProperty>("isNewEdge");
    tlp::BooleanProperty *adjacentToDeletedEdge = newGraph->getLocalProperty<tlp::BooleanProperty>("adjDeletedEdge");
    tlp::ColorProperty *colors = newGraph->getLocalProperty<tlp::ColorProperty>("viewColor");     
    isNewNode->setAllNodeValue(false);
    isNewEdge->setAllNodeValue(false);
    adjacentToDeletedEdge->setAllNodeValue(false);

    tlp::node n;
    forEach(n, newGraph->getNodes()) {
        if (!isInIterator<tlp::node>(oldGraph->getNodes(), n)) {
            isNewNode->setNodeValue(n, true);
            colors->setNodeValue(n, m_newColor);
        }
    }

    tlp::edge e;
    forEach(e, newGraph->getEdges()) {
        if (!isInIterator<tlp::edge>(oldGraph->getEdges(), e)) {
            isNewEdge->setEdgeValue(e, true);
            colors->setEdgeValue(e, m_newColor);
        }
    }

    tlp::node source;
    tlp::node target;
    forEach(e, oldGraph->getEdges()) {
        if (!isInIterator<tlp::edge>(newGraph->getEdges(), e)) {
            isNewEdge->setEdgeValue(e, true);
            source = oldGraph->source(e);
            target = oldGraph->target(e);
            if (isInIterator<tlp::node>(newGraph->getNodes(), source)) {
                adjacentToDeletedEdge->setNodeValue(source, true);
                colors->setNodeValue(source, m_adjToDeletedColor);
            }
            if (isInIterator<tlp::node>(newGraph->getNodes(), target)) {
                adjacentToDeletedEdge->setNodeValue(target, true);
                colors->setNodeValue(target, m_adjToDeletedColor);
            }
        }
    }
    return true;    
}

bool FMMMIncremental::positionNodes(tlp::Graph *g) {
    tlp::BooleanProperty *isNewNode = g->getLocalProperty<tlp::BooleanProperty>("isNewNode");
    tlp::BooleanProperty *isNewEdge = g->getLocalProperty<tlp::BooleanProperty>("isNewEdge");    
    tlp::BooleanProperty *adjacentToDeletedEdge = g->getLocalProperty<tlp::BooleanProperty>("adjDeletedEdge");
    tlp::BooleanProperty *canMove = g->getLocalProperty<tlp::BooleanProperty>("canMove");
    tlp::BooleanProperty *positioned = g->getLocalProperty<tlp::BooleanProperty>("positioned");
    tlp::LayoutProperty *pos = g->getLocalProperty<tlp::LayoutProperty>("viewLayout");
    tlp::DoubleProperty *rot = g->getLocalProperty<tlp::DoubleProperty>("viewRotation");
    tlp::SizeProperty *size = g->getLocalProperty<tlp::SizeProperty>("viewSize");
    std::vector<tlp::node> newNodes;
    canMove->setAllNodeValue(false);
    positioned->setAllNodeValue(false);
    tlp::node n;
    tlp::node n2;
    tlp::BoundingBox bb = tlp::computeBoundingBox(g, pos, size, rot);
    std::srand(std::time(nullptr));

    forEach(n, g->getNodes()) {
        std::cout << pos->getNodeValue(n) << std::endl;
        if (adjacentToDeletedEdge->getNodeValue(n))
            canMove->setNodeValue(n, true);
        if (!isNewNode->getNodeValue(n))
            positioned->setNodeValue(n, true);
        else
            newNodes.push_back(n);
    }

    tlp::edge e;
    forEach(e, g->getEdges()) {
        if (isNewEdge->getEdgeValue(e)) {
            canMove->setNodeValue(g->source(e), true);
            canMove->setNodeValue(g->target(e), true);
        }
    }

    tlp::Graph *newNodeSg = g->inducedSubGraph(newNodes);
    std::vector<std::vector<tlp::node>> components;
    tlp::ConnectedTest::computeConnectedComponents(newNodeSg, components);

    for (auto cc : components) {
        if (cc.size() == 0) 
            return false;

        tlp::Graph *ccSg =  newNodeSg->inducedSubGraph(cc);

        // find the node with the most positioned neighbors in the connected component, and position the new nodes in a bfs-way from this node
        int maxPositionedNeighbors = -1;
        tlp::node maxPositionedNeighborsNode = cc[0];
        for (auto n : cc) {
            std::vector<tlp::node> positionedNeighbors;
            forEach(n2, ccSg->getInOutNodes(n)) {
                if (positioned->getNodeValue(n2))
                    positionedNeighbors.push_back(n2);
            }
            if ((int)positionedNeighbors.size() > maxPositionedNeighbors) {
                maxPositionedNeighborsNode = n;
                maxPositionedNeighbors = positionedNeighbors.size(); 
            }
        }

        forEach (n, ccSg->bfs(maxPositionedNeighborsNode)) {
            canMove->setNodeValue(n, true);
            
            std::vector<tlp::node> positionedNeighbors;
            forEach(n2, ccSg->getInOutNodes(n)) {
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
                tlp::Vec3f sumPos(0);
                for (auto neighbor : positionedNeighbors) {
                    sumPos += pos->getNodeValue(neighbor);
                    canMove->setNodeValue(neighbor, true);
                }
                sumPos /= (float)nbPositionedNeighbors;
                pos->setNodeValue(n, sumPos);
            }
            positioned->setNodeValue(n, true);
        }
    }
    g->delAllSubGraphs(newNodeSg);
    return true;
}

#ifndef FMMMINCREMENTAL_REGISTERED
#define FMMMINCREMENTAL_REGISTERED
PLUGIN(FMMMIncremental)
#endif