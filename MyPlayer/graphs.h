#ifndef GRAPHS_H
#define GRAPHS_H

typedef struct AdjListNode {
    short dest;
    struct AdjListNode * next;
} AdjListNode;

typedef struct AdjList {
    short length;
    AdjListNode * head;
} AdjList;

typedef struct SCGraph { 
    short numNodes;
    short numArcs;

    // Nodes have id's 0 < id < numArcs
    // So this table maps them to the index of their respective box
    short nodeToBox[NUM_BOXES];

    // Edges are stored in these lists
    // e.g.
    // 0 -> 1, 2
    // 1 -> 0
    // 2 -> 0
    AdjList * adjLists;
} SCGraph;

void unscoredStateToSCGraph(SCGraph * graph, const UnscoredState * state);
void newAdjLists(SCGraph * graph);
void freeAdjLists(SCGraph * graph);
void copySCGraph(SCGraph * destGraph, const SCGraph * srcGraph);
short getGraphsPotentialMoves(const SCGraph * graph, Edge * potentialMoves);
void runGraphsTests();

#endif
