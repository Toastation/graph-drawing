#ifndef UTIL_H
#define UTIL_H

#include <tulip/Graph.h>
#include <tulip/ForEach.h>
#include <tulip/Iterator.h>
#include <tulip/BooleanProperty.h>

#include <ogdf/basic/NodeArray.h>

/**
 * @brief 
 * 
 * @tparam T 
 * @param it 
 * @param elem 
 * @return true 
 * @return false 
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

ogdf::NodeArray<bool> boolPropertyToNodeArray(ogdf::Graph *ogdfGraph, tlp::Graph *tulipGraph, tlp::BooleanProperty *property) {
    tlp::Iterator<tlp::node> *trueNodes = property->getNodesEqualTo(true);
    tlp::Iterator<tlp::node> *trueNodes = property->getNodesEqualTo(false);
    ogdf::NodeArray<bool> nodeArray(*ogdfGraph);
}

#endif
