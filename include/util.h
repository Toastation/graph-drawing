#ifndef UTIL_H
#define UTIL_H

#include <tulip/Graph.h>
#include <tulip/Iterator.h>

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
