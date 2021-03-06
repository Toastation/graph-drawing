#include <iostream>
#include <fstream>

#include <tulip/TlpTools.h>
#include <tulip/Graph.h>
#include <tulip/ForEach.h>
#include <tulip/DataSet.h>
#include <tulip/LayoutProperty.h>

int main() {
    tlp::initTulipLib();
    
    // std::ofstream out("out.txt");
    // std::cout.rdbuf(out.rdbuf());

    tlp::Graph *graph = tlp::loadGraph("C:/Users/Melvin.Melvin-PC/Desktop/work/graph-drawing/dataset/incremental2.json");

    std::string errorMessage;
    tlp::LayoutProperty *layout = graph->getLocalProperty<tlp::LayoutProperty>("viewLayout");
    tlp::DataSet ds;
    // ds.set("adaptive cooling", false);
    // ds.set("stopping criterion", true);
    // ds.set("refinement", true);
    // ds.set("refinement iterations", 20);
    // ds.set("refinement frequency", 20);    
    // graph->applyAlgorithm("Incremental", errorMessage);
    graph->applyPropertyAlgorithm("Custom Layout", layout, errorMessage, nullptr, &ds);

    if (!errorMessage.empty())
        std::cout << errorMessage << std::endl;
    
    delete graph;
    std::cout << "goodbye!" << std::endl;
    return EXIT_SUCCESS;
}