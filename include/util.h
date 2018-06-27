#ifndef UTIL_H
#define UTIL_H

#include <tulip/Graph.h>
#include <tulip/ForEach.h>
#include <tulip/Iterator.h>
#include <tulip/BooleanProperty.h>
#include <tulip/TulipToOGDF.h>

#include <ogdf/basic/Graph_d.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/module/LayoutModule.h>



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

/**
 * @brief Converts a tlp::BooleanProperty to a ogdf::NodeArray<bool>
 * @param ogdfGraph The ogdf graph
 * @param tulipGraph The tulip graph
 * @param property The boolean property
 * @return ogdf::NodeArray<bool> The equivalent NodeArray
 */
ogdf::NodeArray<bool> boolPropertyToNodeArray(ogdf::TulipToOGDF *tlpToOGDF, tlp::BooleanProperty *property) {
    tlp::Graph &tlpGraph = tlpToOGDF->getTlp();
    ogdf::Graph &ogdfGraph = tlpToOGDF->getOGDFGraph();
    ogdf::NodeArray<bool> nodeArray(ogdfGraph);
    const std::vector<tlp::node> &nodes = tlpGraph.nodes();
    for (int i = 0; i < nodes.size(); i++) {
        nodeArray[tlpToOGDF->getOGDFGraphNode(i)] = property->getNodeValue(nodes[i]);
    }
}

#endif
