#ifndef UTIL_H
#define UTIL_H

#include <tulip/Graph.h>
#include <tulip/ForEach.h>
#include <tulip/Iterator.h>
#include <tulip/BooleanProperty.h>

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

#endif
