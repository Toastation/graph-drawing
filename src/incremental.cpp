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

Incremental::Incremental(const tlp::PluginContext* context) 
    : tlp::Algorithm(context), m_idealEdgeLength(DEFAULT_IDEAL_EDGE_LENGTH), m_newColor(DEFAULT_NEW_COLOR), m_adjToDeletedColor(DEFAULT_ADJ_TO_DELETED_COLOR) {
    addInParameter<bool>("adaptive cooling", "If true, the algo uses a local cooling function based on the angle between each node's movement. Else it uses a global linear cooling function.", "", false);
	addInParameter<bool>("stopping criterion", "If true, stops the algo before the maximum number of iterations if the graph has converged. See \"convergence threshold\"", "", false);
	addInParameter<bool>("multipole expansion", "If true, apply a 4-term multipole expansion for more accurate layout. May affect performances.", "", false);
	addInParameter<bool>("refinement", "", "", false);	
    addInParameter<bool>("pack CC", "pack connected components", "", false);
	addInParameter<unsigned int>("max iterations", "The maximum number of iterations of the algorithm.", "300", false);
	addInParameter<unsigned int>("max displacement", "The maximum length a node can move. Very high values or very low values may result in chaotic behavior.", "200", false);
	addInParameter<unsigned int>("refinement iterations", "", "20", false);
	addInParameter<unsigned int>("refinement frequency", "", "30", false);	
	addInParameter<float>("ideal edge length", "The ideal edge length.", "10", false);
	addInParameter<float>("spring force strength", "Factor of the spring force", "1", false);
	addInParameter<float>("repulsive force strength", "Factor of the repulsive force", "100", false);
	addInParameter<float>("convergence threshold", "If the average node energy is lower than this threshold, the graph is considered to have converged and the algorithm stops. Only taken into consideration if \"stopping criterion\" is true", "0.1", false);
    addInParameter<float>("high energy threshold", "Threshold above which a node is consired to have a high energy", "1.0", false);	
	addInParameter<float>("center attraction strength", "Strength of the attraction of nodes toward the center", "0.000001f", false);	
    addDependency("Custom Layout", "1.0");
}

bool Incremental::check(std::string &errorMessage) {
    std::srand(std::time(NULL));
    return true;
}

bool Incremental::run() {
    init();

    if (graph->numberOfSubGraphs() == 0)
        pluginProgress->setError("There is no timeline!");

    std::vector<tlp::Graph *> subgraphs;
    std::string errorMessage;
    tlp::Graph *g;
    forEach (g, graph->getSubGraphs()) {
        subgraphs.push_back(g);
    }

    // TODO: we should check the order of the subgraphs, right now it's the responsability of the user

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
        if (i > 0) { // no need to block nodes and compute differences for the first graph of the timeline
            computeDifference(subgraphs[i-1], subgraphs[i]);
            positionNodes(subgraphs[i], subgraphs[i-1]);
            ds.set("block nodes", true);
            ds.set("movable nodes", subgraphs[i]->getLocalProperty<tlp::BooleanProperty>("canMove"));
        }
        ds.set("pack connected components", m_packCC ? i % 20 == 0 : false); 
        subgraphs[i]->applyPropertyAlgorithm("Custom Layout", currentPos, errorMessage, nullptr, &ds);
        previousPos = currentPos;
        pluginProgress->progress(i, subgraphs.size());
    }
    return true;
}

void Incremental::init() {
    m_packCC = false;
	bool btemp = false;
	int itemp = 0;
	float ftemp = 0.0f;
	if (dataSet != nullptr) {
		if (dataSet->get("max iterations", itemp))
			ds.set("max iterations", itemp);
		if (dataSet->get("refinement iterations", itemp))
			ds.set("refinement iterations", itemp);
		if (dataSet->get("refinement frequency", itemp))
			ds.set("refinement frequency", itemp);
		if (dataSet->get("max displacement", ftemp))
			ds.set("max displacement", ftemp);
		if (dataSet->get("ideal edge length", ftemp))
			ds.set("ideal edge length", ftemp);
		if (dataSet->get("spring force strength", ftemp))
			ds.set("spring force strength", ftemp);
		if (dataSet->get("repulsive force strength", ftemp))
			ds.set("repulsive force strength", ftemp);
		if (dataSet->get("convergence threshold", ftemp))
			ds.set("convergence threshold", ftemp);
		if (dataSet->get("high energy threshold", ftemp))
			ds.set("high energy threshold", ftemp);
        if (dataSet->get("center attraction strength", ftemp))
			ds.set("center attraction strength", ftemp);
		if (dataSet->get("adaptive cooling", btemp))
			ds.set("adaptive cooling", btemp);
		if (dataSet->get("stopping criterion", btemp))
			ds.set("stopping criterion", btemp);
		if (dataSet->get("multipole expansion", btemp))
			ds.set("multipole expansion", btemp);
		if (dataSet->get("refinement", btemp))
			ds.set("refinement", btemp);
        if (dataSet->get("pack CC", btemp))
            m_packCC = btemp;
	}
}

bool Incremental::computeDifference(tlp::Graph *oldGraph, tlp::Graph *newGraph) {
    tlp::BooleanProperty *isNewNode = newGraph->getLocalProperty<tlp::BooleanProperty>("isNewNode");
    tlp::BooleanProperty *isNewEdge = newGraph->getLocalProperty<tlp::BooleanProperty>("isNewEdge");
    tlp::BooleanProperty *adjacentToDeletedEdge = newGraph->getLocalProperty<tlp::BooleanProperty>("adjDeletedEdge");
    tlp::ColorProperty *colors = newGraph->getLocalProperty<tlp::ColorProperty>("viewColor");     
    isNewNode->setAllNodeValue(false);
    isNewEdge->setAllNodeValue(false);
    adjacentToDeletedEdge->setAllNodeValue(false);
    tlp::node source;
    tlp::node target;
    tlp::node n;
    tlp::edge e;

    // initialise the hashmaps that are used to find a node/edge exist in constant time
    for (unsigned int i = 0; i < oldGraph->numberOfNodes(); ++i) {
        m_oldGraphNodes[oldGraph->nodes()[i]] = true;
    }
    for (unsigned int i = 0; i < newGraph->numberOfNodes(); ++i) {
        m_newGraphNodes[newGraph->nodes()[i]] = true;
    }
    for (unsigned int i = 0; i < oldGraph->numberOfEdges(); ++i) {
        m_oldGraphEdges[oldGraph->edges()[i]] = true;
    }
    for (unsigned int i = 0; i < newGraph->numberOfEdges(); ++i) {
        m_newGraphEdges[newGraph->edges()[i]] = true;
    }

    // mark new nodes
    forEach(n, newGraph->getNodes()) {
        if (m_oldGraphNodes.find(n) == m_oldGraphNodes.end()) {
            isNewNode->setNodeValue(n, true);
            colors->setNodeValue(n, m_newColor);
        }
    }

    // mark new edges
    forEach(e, newGraph->getEdges()) {
        if (m_oldGraphEdges.find(e) == m_oldGraphEdges.end()) {
            isNewEdge->setEdgeValue(e, true);
            colors->setEdgeValue(e, m_newColor);
        }
    }

    // mark nodes who lost a neighbor
    forEach(e, oldGraph->getEdges()) {
        if (m_newGraphEdges.find(e) == m_newGraphEdges.end()) {
            isNewEdge->setEdgeValue(e, true);
            source = oldGraph->source(e);
            target = oldGraph->target(e);
            if (m_newGraphNodes.find(source) != m_newGraphNodes.end()) {
                adjacentToDeletedEdge->setNodeValue(source, true);
                colors->setNodeValue(source, m_adjToDeletedColor);
            }
            if (m_newGraphNodes.find(target) != m_newGraphNodes.end()) {
                adjacentToDeletedEdge->setNodeValue(target, true);
                colors->setNodeValue(target, m_adjToDeletedColor);
            }
        }
    }
    return true;    
}

bool Incremental::positionNodes(tlp::Graph *g, tlp::Graph *previous) {
    tlp::BooleanProperty *isNewNode = g->getLocalProperty<tlp::BooleanProperty>("isNewNode");
    tlp::BooleanProperty *isNewEdge = g->getLocalProperty<tlp::BooleanProperty>("isNewEdge");    
    tlp::BooleanProperty *adjacentToDeletedEdge = g->getLocalProperty<tlp::BooleanProperty>("adjDeletedEdge");
    tlp::BooleanProperty *canMove = g->getLocalProperty<tlp::BooleanProperty>("canMove");
    tlp::BooleanProperty *positioned = g->getLocalProperty<tlp::BooleanProperty>("positioned");
    tlp::LayoutProperty *pos = g->getLocalProperty<tlp::LayoutProperty>("viewLayout");
    tlp::LayoutProperty *posPrev = previous->getLocalProperty<tlp::LayoutProperty>("viewLayout");
    tlp::DoubleProperty *rotPrev = previous->getLocalProperty<tlp::DoubleProperty>("viewRotation");
    tlp::SizeProperty *sizePrev = previous->getLocalProperty<tlp::SizeProperty>("viewSize");
    std::vector<tlp::node> newNodes;
    canMove->setAllNodeValue(false);
    positioned->setAllNodeValue(false);
    tlp::edge e;
    tlp::node n;
    tlp::node n2;
    tlp::BoundingBox bb = tlp::computeBoundingBox(previous, posPrev, sizePrev, rotPrev);
    
    // mark already positioned nodes, and list the new nodes
    forEach(n, g->getNodes()) {
        if (adjacentToDeletedEdge->getNodeValue(n))
            canMove->setNodeValue(n, true);
        if (!isNewNode->getNodeValue(n))
            positioned->setNodeValue(n, true);
        else
            newNodes.push_back(n);
    }

    // allow nodes connected to a new edge to move
    forEach(e, g->getEdges()) {
        if (isNewEdge->getEdgeValue(e)) {
            canMove->setNodeValue(g->source(e), true);
            canMove->setNodeValue(g->target(e), true);
        }
    }

    // create a subgraph from the new nodes and store the connected components of this subgraph
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
                float randomAngle = (float(std::rand()) / float(RAND_MAX)) * float(TAU); 
                pos->setNodeValue(n, bb.center() + tlp::Vec3f(std::cos(randomAngle), std::sin(randomAngle)));
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
PLUGIN(Incremental)
#endif