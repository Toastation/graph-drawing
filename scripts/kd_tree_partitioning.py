from tulip import tlp
from collections import deque
import math
import random

ITERATIONS = 4
PARTITION_SIZE = 5

def main(graph):
    layout = graph.getLayoutProperty("viewLayout")
    colors = graph.getColorProperty("viewColor")
    partitions = deque([graph.nodes()])

    quit = False
    count = 0

    while not quit:
        nb_partitions = len(partitions)
        for i in range(nb_partitions):
            p = partitions.popleft()
            p_size = len(p)
            
            # sort the partition by x and y coord alternatively
            p.sort(key=lambda pos: layout[pos].x()) if count % 2 == 0 else p.sort(key=lambda pos: layout[pos].y()) 

            # cut the partition in two and push the 2 new partitions in the queue
            median_index = p_size // 2
            partitions.append(p[:median_index])
            partitions.append(p[median_index:])
            
            # terminate when we have reached the ideal partition size 
            if p_size // 2 <= PARTITION_SIZE: 
                quit = True
                
        count += 1
    
    partitions = list(partitions)
    
    # DEBUG
    print(partitions)
    for p in partitions:
        r = random.randrange(255)
        g = random.randrange(255)
        b = random.randrange(255)
        for n in p:
            colors[n] = tlp.Color(r, g, b)  
