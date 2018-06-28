#include <iostream>

#include <tulip/TlpTools.h>
#include <tulip/Graph.h>
#include <tulip/ForEach.h>

int main() {
    tlp::initTulipLib();

    tlp::Graph *graph = tlp::loadGraph("C:/Users/Melvin.Melvin-PC/Desktop/work/graph-drawing/graphs/other/test.json");

    std::string errorMessage;
    graph->applyAlgorithm("FM^3 incremental", errorMessage);

    if (!errorMessage.empty()) {
        std::cout << errorMessage << std::endl;
    }

    return EXIT_SUCCESS;
}

// #include <iostream>
// #include <ogdf/basic/NodeArray.h>

// int main() {
//     ogdf::NodeArray<bool> nodeArray();

//     return EXIT_SUCCESS;
// }