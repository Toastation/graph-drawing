### Prerequisites
You first need to compile the static layout algorithm. Place the files ```src/custom_layout.cpp```, ```src/custom_layout.h```, ```src/incremental.cpp``` and ```src/incremental.h``` in the externalplugins directory of the tulip sources, and recompile.

---

### How to use:
Use the plugin "Incremental" on the root node of a timeline to draw all the steps. You can also use the plugin "Custom Layout" to draw a static layout of a graph.

Use the script ```scripts/morph.py``` on the root node of a timeline (already processed by "Incremental") to run an animation. The animation stops at each steps so be sure to press continue.


---

### Graph hierarchy: 
The plugin "Incremental" should only be called on the root graph of a timeline. The root graph must contain all the nodes/edges of every graph of the timeline. The subgraphs of the root graph correspond to the different steps of the timeline. They must be ordered by their id.
