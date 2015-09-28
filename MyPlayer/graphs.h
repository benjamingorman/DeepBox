#ifndef GRAPHS_H
#define GRAPHS_H

typedef struct SCGraph { 
    short numNodes;
    short numArcs;

    // Nodes have id's 0 < id < numArcs
    // So this table maps them to the index of their respective box
    short nodeToBox[NUM_BOXES];

    // Heap allocated on initialization to save memory. Size is numNodes x numNodes
    // Keep it as a contiguous array for memory efficiency.
    char *adjMat;
} SCGraph;

void unscoredStateToSCGraph(SCGraph * graph, const UnscoredState * state);
void runGraphsTests();

#endif
