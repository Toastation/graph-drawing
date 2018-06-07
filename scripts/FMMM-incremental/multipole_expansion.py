from tulip import tlp
from anytree import NodeMixin, RenderTree, AsciiStyle, PreOrderIter
import cmath
import math
import random

P = 4   # P-term multipole expansion
VERTICES_THRESHOLD = 200 # max number of vertices in the leaves of the kd-tree

class KDTree(NodeMixin):

    def __init__(self, parent, subgraph, center, radius):
        super(NodeMixin, self).__init__()
        self.parent = parent
        self.subgraph = subgraph
        self.center = center
        self.radius = radius

class MultipoleExpansion:

    def __init__(self):
        self._precision = P
        self._vertices_threshold = VERTICES_THRESHOLD

    ## \brief Computes the coefficient of the multipole expansion of a node
    # \param pos The viewLayout of the graph
    # \param node A node of the kd-tree
    # \return A list of complex numbers
    def _compute_coef(self, pos, node):
        z_0 = complex(node.center.x(), node.center.y())
        coef = [complex(0, 0), complex(0, 0), complex(0, 0), complex(0, 0), complex(0, 0)]
        coef[0] += node.subgraph.numberOfNodes() # Q
        for v in node.subgraph.getNodes():
            z_v = complex(pos[v].x(), pos[v].y())
            z_v_minus_z0_over_k = z_v - z_0
            for k in range(1, self._precision + 1):
                coef[k] += (-1.0) * z_v_minus_z0_over_k / k
                z_v_minus_z0_over_k *= z_v - z_0
        node.coef = coef

    ## \brief Builds recursively the kd-tree under a node, if the computed children have a number of vertices less than the threshold,
    # stop the recusion.
    # \param pos The viewLayout of the graph 
    # \param node The node from which to construct the kd-tree
    # \param level The current depth of the tree, used for determining on which axis we should split the vertices
    def _build_children(self, pos, node, level):
        vertices = node.subgraph.nodes()
        vertices.sort(key = lambda n : pos[n].x()) if level % 2 == 0 else vertices.sort(key = lambda n : pos[n].y())
        median_index = len(vertices) // 2
        left_subgraph = node.subgraph.inducedSubGraph(vertices[:median_index])
        right_subgraph = node.subgraph.inducedSubGraph(vertices[median_index:])
        (left_center, left_farthest_point) = tlp.computeBoundingRadius(left_subgraph)
        (right_center, right_farthest_point) = tlp.computeBoundingRadius(right_subgraph)       
        left_child = KDTree(node, left_subgraph, left_center, left_center.dist(left_farthest_point))
        right_child = KDTree(node, right_subgraph, right_center, right_center.dist(right_farthest_point))
        self._compute_coef(pos, left_child)  # TODO: compute them after with multipole expansion shifting from the leaves
        self._compute_coef(pos, right_child)
        if max(left_subgraph.numberOfNodes(), right_subgraph.numberOfNodes()) > self._vertices_threshold:
            self._build_children(pos, left_child, level + 1)
            self._build_children(pos, right_child, level + 1)            
        else:
            return 

    ## \brief Builds a 2D-tree from the given graph, where each node of the tree store a subgraph and its corresponding coefficients of the 
    # multipole expansion. The subgraphs are evenly split from the median node of the x/y axis alternatively.
    # \param graph The graph to compute the KD-tree from
    # \return The root of the tree
    def build_tree(self, graph):
        for g in graph.getSubGraphs():
            graph.delAllSubGraphs(g)
        pos = graph.getLayoutProperty("viewLayout")
        (center, farthest_point) = tlp.computeBoundingRadius(graph)
        #print("center: {} | farthest point: {} | dist: {}".format(center, farthest_point, center.dist(farthest_point)))
        root = KDTree(None, graph, center, center.dist(farthest_point))
        if graph.numberOfNodes() <= self._vertices_threshold: return root
        self._compute_coef(pos, root)
        self._build_children(pos, root, 0)
        self.debug(graph, root)
        return root

    def debug(self, graph, tree):
        color = graph.getColorProperty("viewColor")
        nodes = [n for n in PreOrderIter(tree)]
        leaves = [n for n in nodes if n.is_leaf]
        for l in leaves:
            rcolor = tlp.Color(random.randint(0, 255), random.randint(0, 255), random.randint(0, 255))
            for v in l.subgraph.getNodes():
                color[v] = rcolor

def main(graph):
    m = MultipoleExpansion()
    root = m.build_tree(graph)
    #m.debug(graph, root)
    #print(RenderTree(root, style=AsciiStyle()).by_attr("radius"))

        

                
