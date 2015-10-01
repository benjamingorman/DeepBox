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
    short nodeToBox[NUM_BOXES+1]; // because the outside of the board is classed as an 'imaginary' node
    short boxToNode[NUM_BOXES];

    // Edges are stored in these lists
    // e.g.
    // 0 -> 1, 2
    // 1 -> 0
    // 2 -> 0
    AdjList * adjLists;
} SCGraph;

typedef struct GMCTSNode {
    struct GMCTSNode * parent;

    int visits;
    float score;

    Edge move;
    short numBoxesTakenByMove;

    Edge potentialMoves[NUM_EDGES];
    short numPotentialMoves;
    short nextPotentialMoveIndex;

    struct GMCTSNode * children[NUM_EDGES];
    short numChildren;
} GMCTSNode;

void unscoredStateToSCGraph(SCGraph * graph, const UnscoredState * state);
void newAdjLists(SCGraph * graph);
void freeAdjLists(SCGraph * graph);
void copySCGraph(SCGraph * destGraph, const SCGraph * srcGraph);
short getGraphsPotentialMoves(const SCGraph * graph, Edge * potentialMoves);
short getNodeValency(const SCGraph * graph, short node);
short getSuperGraphUrgentMoves(const SCGraph * graph, Edge * movesBuf);
void removeConnectionEdge(SCGraph * graph, Edge edge);
short getNumNodesLeftToCapture(const SCGraph * graph);
void runGraphsTests();
Edge getGraphsMonteCarloMove(const UnscoredState * rootState, int maxRuntime);

#endif
