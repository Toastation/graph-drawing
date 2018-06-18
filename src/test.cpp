#include <iostream>
#include <tulip/TlpTools.h>
#include <tulip/Graph.h>

int main() {
    tlp::initTulipLib();

    tlp::Graph *graph = tlp::newGraph();

    tlp::node n1 = graph->addNode();
    tlp::node n2 = graph->addNode();
    tlp::node n3 = graph->addNode();

    tlp::edge e1 = graph->addEdge(n2, n3);
    tlp::edge e2 = graph->addEdge(n1, n2);
    tlp::edge e3 = graph->addEdge(n3, n1);

    std::cout << graph << std::flush;

    delete graph;

    return EXIT_SUCCESS;
}