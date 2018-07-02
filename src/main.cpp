#include <iostream>

#include <tulip/TlpTools.h>
#include <tulip/Graph.h>
#include <tulip/ForEach.h>
#include <tulip/DataSet.h>
#include <tulip/LayoutProperty.h>

int main() {
    tlp::initTulipLib();

    tlp::Graph *graph = tlp::loadGraph("C:/Users/Melvin.Melvin-PC/Desktop/work/graph-drawing/graphs/other/n1000.json");
    tlp::saveGraph(graph, "before.tlp");

    std::string errorMessage;
    tlp::LayoutProperty *layout = graph->getLocalProperty<tlp::LayoutProperty>("viewLayout");
    graph->applyPropertyAlgorithm("FM^3 (custom)", layout, errorMessage);

    if (!errorMessage.empty()) {
        std::cout << errorMessage << std::endl;
    }
    tlp::saveGraph(graph, "after.tlp");
    delete graph;
    std::cout << "goodbye!" << std::endl;
    return EXIT_SUCCESS;
}

// #include <iostream>

// int main() {
//     for (int i = 0; i < 10; i++) {
//         std::cout << "test " << i << std::endl;
//     }
//     return EXIT_SUCCESS;
// }