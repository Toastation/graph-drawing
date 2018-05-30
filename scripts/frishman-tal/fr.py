    layout = graph.getLayoutProperty("viewLayout")
    disp = graph.getLayoutProperty("disp")

    bounding_box = tlp.computeBoundingBox(graph)

    n = graph.numberOfNodes()
    w = bounding_box.width() + k
    h = bounding_box.height() + k
    k_sq = k * k
    min_dist = 10e-6
    min_dist_sq = min_dist * min_dist
    c_rep = 0.052
    conv_tolerance = 0.01

    # scaling so that area = n * k^2    
    ratio = h/w
    W = math.sqrt(n / ratio) * k
    H = ratio * W

    temp_x = W / 8
    temp_y = H / 8
    cooling_factor = 0.95

    converged = (iterations == 0)
    it_count = 1

    while not converged:
        # repulsive forces
        for u in graph.getNodes():
            for v in graph.getNodes():
                if u == v: continue
                dist = layout[u] - layout[v]
                dist_sq = max(min_dist_sq, dist.x() * dist.x() + dist.y() * dist.y())
                dist *= 1 / dist_sq
                disp[u] += dist
            disp[u] *= c_rep

        # attractive forces
        for e in graph.getEdges():
            u = graph.source(e)
            v = graph.target(e)            
            dist = layout[u] - layout[v]
            dist_norm = min(min_dist, dist.norm())
            disp[u] -= dist * (dist_norm / k)
            disp[v] += dist * (dist_norm / k)

        # update pos
        for u in graph.getNodes():
            dist_norm = disp[u].norm()
            dx = disp[u].x() / dist_norm * min(dist_norm, temp_x)
            dy = disp[u].y() / dist_norm * min(dist_norm, temp_y)
            d_sq = dx * dx + dy * dy
            threshold = conv_tolerance * k
            if d_sq > threshold * threshold: converged = False
            layout[u] += tlp.Vec3f(dx, dy, 0)
        temp_x *= cooling_factor
        temp_y *= cooling_factor
        it_count += 1
        converged = (it_count > iterations) or converged