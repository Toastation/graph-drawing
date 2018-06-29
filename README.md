### How to use:
To draw the latest graph in the timeline, run ```scripts/fmmm_incremental.py``` on the root graph.

---

### Graph hierarchy: 
```scrips/fmmm_incremental.py``` should only be called on the root graph. The root graph must contain all the nodes/edges of every graph of the timeline. The subgraphs of the root graph correspond to the differents steps of the timeline. They must be ordered by their id.

---

/!\ Currently, this script only works for online graph drawing, i.e when the script is called, it will compute the layout from the latest graph in the timeline based on the previous one.
