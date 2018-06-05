from tulip import tlp
from multipole_expansion import KDTree, MultipoleExpansion
import math

DEFAULT_ITERATIONS = 300
REPL_CONST = 4.0
DL = 20

class FMMMLayout:

    def __init__(self):
        self._default_iterations = DEFAULT_ITERATIONS
        self._repl_const = REPL_CONST
        self._desired_edge_length = DL
        self._init_t = 200
        self._cooling_factor = 0.9
        self._multipole_exp = MultipoleExpansion()

    def _repulsive_force(self, dist):
        dist_norm = dist.norm()
        if dist_norm == 0: return tlp.Vec3f() # TODO: push nodes
        return (self._repl_const * dist) / (dist.norm() ** 3)

    def _attractive_force(self, dist):
        dist_norm = dist.norm()
        return dist_norm * math.log(dist_norm / self._desired_edge_length) * dist

    def _compute_repulsive_forces(self, graph, vertex, node):
        pos = graph.getLayoutProperty("viewLayout")
        disp = graph.getLayoutProperty("disp")
        def aux(node):
            dist = pos[vertex] - node.center
            if dist.norm() > node.radius: # compute approximate repl forces
                disp[vertex] += node.subgraph.numberOfNodes() * self._repulsive_force(dist)
                return
            else:
                if node.is_leaf: # compute exact repl forces
                    for v in node.vertices:
                        if v != vertex:
                            d = pos[vertex] - pos[v]
                            disp[vertex] += self._repulsive_force(d)
                    return
                else:
                    aux(node.children[0])
                    aux(node.children[1])
        aux(node)

    def run(self, graph, iterations = DEFAULT_ITERATIONS):
        pos = graph.getLayoutProperty("viewLayout")
        disp = graph.getLayoutProperty("disp")
        t = self._init_t
        for i in range(iterations):
            if i <= 3 or i % 20 == 0: # recompute the tree for the first 4 iterations and then every 20 iterations
                kd_tree = self._multipole_exp.build_tree(graph)
        
            for n in graph.getNodes():
                self._compute_repulsive_forces(graph, n, kd_tree)
                for neighbor in graph.getInOutNodes(n):
                    dist = pos[n] - pos[neighbor]
                    disp[n] -= self._attractive_force(dist)
                    disp[neighbor] += self._attractive_force(dist)          
          
            for n in graph.getNodes():
                disp_norm = disp[n].norm()
                if disp_norm != 0:
                    disp[n] = (disp[n] / disp_norm) * min(disp_norm, t)
                pos[n] += disp[n]
                disp[n] = tlp.Vec3f()
            t *= self._cooling_factor
            # TODO: conv threshold?
                
def main(graph):
    l = FMMMLayout()
    l.run(graph)
