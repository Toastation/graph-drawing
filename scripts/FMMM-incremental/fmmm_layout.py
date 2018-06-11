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
        self._cooling_factor = 0.95
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
                    for v in node.subgraph.getNodes():
                        if v != vertex:
                            d = pos[vertex] - pos[v]
                            force = self._repulsive_force(d)
                            disp[vertex] += force
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

            for e in graph.getEdges():
                source = graph.source(e)
                target = graph.target(e)
                dist = pos[source] - pos[target]
                force = self._attractive_force(dist)
                disp[source] -= force
                disp[target] += force
          
            for n in graph.getNodes():
                disp_norm = disp[n].norm()
                if disp_norm != 0:
                    disp[n] = (disp[n] / disp_norm) * min(disp_norm, t)
                pos[n] += disp[n]
                disp[n] = tlp.Vec3f()
            t *= self._cooling_factor
            #updateVisualization()
            # TODO: conv threshold?

class FMMMLayout2():
    
    def __init__(self):
        self._multipole_exp = MultipoleExpansion()
        self._init_constants()

    def _init_constants(self):
        self.L = 10
        self.K_r = 6250
        self.K_s = 1
        self.R = 0.05
        self.init_t = 200 # cst = 0.04
        self.t_f = 0.9
        self.max_partition_size = 20

    def set_init_temp(self, init_temp):
        if init_temp < 0:
            print("Initial temperature must be positive")
            return
        self.init_t = init_temp

    def _repulsive_force(self, dist_vec):
        dist_norm = dist_vec.norm()
        if dist_norm == 0: return tlp.Vec3f()
        dist_norm_sq = dist_norm * dist_norm
        force = dist_vec * (self.K_r / dist_norm_sq)
        return force / dist_norm

    def _attractive_force(self, dist_vec):
        dist_norm = dist_vec.norm()
        if dist_norm == 0: return tlp.Vec3f()
        force = dist_vec * self.K_s * (dist_norm - self.L)
        return force / dist_norm

    ## \brief Integral of the repulsive force
    def repulsive_force_intgr(self, dist_vec):
        return -(self.K_r / dist_vec.norm())

    ## \brief Integral of the attractive force
    def attractive_force_intgr(self, dist_vec):
        dist_norm = dist_vec.norm()
        return self.K_s * (((dist_norm * dist_norm) / 2) - (self.L * dist_norm)) 

    def _compute_repulsive_forces(self, graph, vertex, node):
        pos = graph.getLayoutProperty("viewLayout")
        disp = graph.getLayoutProperty("disp")
        def aux(node):
            dist = pos[vertex] - node.center    
            if dist.norm() > node.radius: # compute approximate repl forces
                #print("dist norm: {}  |  radius: {}  |  vertex: {}".format(dist.norm(), node.radius, vertex))
                disp[vertex] += node.subgraph.numberOfNodes() * self._repulsive_force(dist)
            else:
                if node.is_leaf: # compute exact repl forces
                    count = 0
                    for v in node.subgraph.getNodes():
                        count += 1
                        if v != vertex:
                            d = pos[vertex] - pos[v]
                            disp[vertex] += self._repulsive_force(d)                         
                else:
                    aux(node.children[0])
                    aux(node.children[1])
        aux(node)

    def run(self, graph, iterations = DEFAULT_ITERATIONS, condition=None):
        layout = graph.getLayoutProperty("viewLayout")
        disp = graph.getLayoutProperty("disp")
        t = self.init_t
        conv_threshold = 6 / graph.numberOfNodes()
        quit = False
        it = 1
        while not quit:
            total_disp = 0
            if it <= 4 or it % 20 == 0: # recompute the tree for the first 4 iterations and then every 20 iterations
                kd_tree = self._multipole_exp.build_tree(graph)                
            for n in graph.getNodes():
                if not condition or condition[n]: self._compute_repulsive_forces(graph, n, kd_tree)
            for e in graph.getEdges():
                u = graph.source(e)
                v = graph.target(e)
                dist = layout[u] - layout[v]
                force = self._attractive_force(dist)
                if not condition or condition[u]: disp[u] -= force
                if not condition or condition[v]: disp[v] += force
            for n in graph.getNodes():
                disp_norm = disp[n].norm()
                if disp_norm != 0: disp[n] = (disp[n] / disp_norm) * min(disp_norm, t)
                else: disp[n] = tlp.Vec3f()
                total_disp += min(disp_norm, t)
                layout[n] += disp[n]
                disp[n] = tlp.Vec3f()
                #if total_disp < conv_threshold: quit = True
            quit = it > iterations or quit # maybe infinite if conv_threshold is too low...
            it += 1
            t *= self.t_f
            #updateVisualization()
        print("number if iterations done: {}".format(it))

def main(graph):
    l = FMMMLayout2()
    l.run(graph)
