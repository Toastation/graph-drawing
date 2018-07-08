#include <iostream>

#include <tulip/TlpTools.h>
#include <tulip/Graph.h>
#include <tulip/ForEach.h>
#include <tulip/DataSet.h>
#include <tulip/LayoutProperty.h>

int main() {
    tlp::initTulipLib();

    tlp::Graph *graph = tlp::loadGraph("C:/Users/Melvin.Melvin-PC/Desktop/work/graph-drawing/graphs/other/test.json");

    std::string errorMessage;
    tlp::LayoutProperty *layout = graph->getLocalProperty<tlp::LayoutProperty>("viewLayout");
    tlp::DataSet ds;
    ds.set("linear median", true);
    graph->applyPropertyAlgorithm("FM^3 (custom)", layout, errorMessage, nullptr, &ds);

    if (!errorMessage.empty()) {
        std::cout << errorMessage << std::endl;
    }
    
    delete graph;
    std::cout << "goodbye!" << std::endl;
    return EXIT_SUCCESS;
}