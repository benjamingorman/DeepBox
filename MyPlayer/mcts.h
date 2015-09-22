#ifndef MCTS_H
#define MCTS_H

typedef struct MCTSNode { 
    struct MCTSNode * parent;
    UnscoredState state; // the post-move state
    PlayerNum playerJustMoved;
    PlayerNum nextPlayerToMove;
    Edge move;
    short numBoxesTakenByMove;

    int totalScore; // from perspective of playerJustMoved
    int visits;

    short numPotentialMoves;
    short numChildren;
    struct MCTSNode * child;

    struct MCTSNode * sibling;
} MCTSNode;

/*
typedef struct MCTSNodeListElement {
    MCTSNode * node;
    MCTSNodeListElement * next;
} MCTSNodeListElement;

typedef struct {
    int numElements;
    MCTSNodeListElement * first;
} MCTSNodeList;
*/

void initMCTSNode(MCTSNode *, MCTSNode *, UnscoredState, PlayerNum, Edge);
void updateMCTSNode(MCTSNode *, short);
MCTSNode * addChildToMCTSNode(MCTSNode *, Edge);
Edge getSimpleMCTSMove(UnscoredState *, int iterations);
short doRandomGame(UnscoredState);
MCTSNode * selectChildUCT(MCTSNode * node);
Edge getMCTSMove(UnscoredState *, int iterations, bool saveTree);
void freeMCTSNode(MCTSNode *);
void saveMCTSNodeJSON(MCTSNode * node, const char * filePath);
json_t * MCTSNodeToJSON(MCTSNode * node);
void runMCTSTests();

#endif
