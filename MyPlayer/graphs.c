#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <jansson.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <bliss_C.h>
#include "game_board.h"
#include "util.h"
#include "graphs.h"

static const short SUB_GRAPH_MAX = 20; // the largest number of sub graphs a single board can be split up into
static const short URGENT_MOVE_MAX = 2; // the maximum number of urgent moves that can be returned
static const short NEIGHBOUR_MAX = 24; // the maximum number of neighbours a node can have (the imaginary node can have up to 24) (the imaginary node can have up to 24)

void newAdjLists(SCGraph * graph);
void freeAdjLists(SCGraph * graph);

static AdjListNode * addAdjListNode(AdjList * list, short dest);
static void removeAdjListNode(AdjList * list, short dest);

static void addConnection(SCGraph * graph, short node1, short node2);
static void removeConnection(SCGraph * graph, short node1, short node2);
static short getNumConnectionsBetween(const SCGraph * graph, short node1, short node2);

void copySCGraph(SCGraph * destGraph, const SCGraph * srcGraph);
static void printSCGraph(const SCGraph * graph);
static short getAllArcs(const SCGraph * graph, short * nodeBuf1, short * nodeBuf2);

static bool areNodesConnected(const SCGraph * graph, short node1, short node2);
static short getConnectedNodes(const SCGraph * graph, short node, short * nodeBuffer);
static short getNodeValency(const SCGraph * graph, short node);

static short getUrgentMoves(const SCGraph * graph, Edge * potentialMoves);
static short getNonIsomorphicMoves(const SCGraph * graph, Edge * potentialMoves);
static short getSubGraphs(const SCGraph * superGraph, SCGraph *subGraphBuffer);

void unscoredStateToSCGraph(SCGraph * graph, const UnscoredState * state) {
    // This function will malloc space for the graph's adjacency matrix
    
    Box remainingBoxes[NUM_BOXES];
    short numRemainingBoxes = getRemainingBoxes(state, remainingBoxes);

    log_debug("Got %d remaining boxes.\n", numRemainingBoxes);
    
    // Node 0 in every graph maps to the constant NO_BOX.
    // This is so the bordering edges of a dots and boxes board can be represented in an
    // adjacency matrix.
    graph->numNodes = numRemainingBoxes + 1;
    graph->numArcs = 0;
    
    // Now we know how many nodes there are we can allocate space for the adjacency matrix.
    newAdjLists(graph);
    log_debug("Initialized adjacency lists.\n");

    // Now populate nodeToBox with the correct values.
    Box boxToNode[NUM_BOXES]; // temporary - helps with building adjacency matrix

    graph->nodeToBox[0] = NO_BOX;
    for(short node=1; node < graph->numNodes; node++) {
        Box b = remainingBoxes[node-1];
        graph->nodeToBox[node] = b;
        boxToNode[b] = node;
    }
    log_debug("Initialized nodeToBox.\n");

    // Now fill in the adjacency lists
    for(Edge e=0; e < NUM_EDGES; e++) {
        if(!isEdgeTaken(state, e)) {
            log_debug("Edge %d is not taken.\n", e);

            const Box * edgeBoxes = getEdgeBoxes(e);
            short b1 = edgeBoxes[0];
            short b2 = edgeBoxes[1];

            log_debug("b1: %d, b2: %d\n", b1, b2);

            // Remember 0 can have 2 connections to corner boxes
            if (b1 == NO_BOX) {
                addConnection(graph, 0, boxToNode[b2]);
            }
            else if (b2 == NO_BOX) {
                addConnection(graph, boxToNode[b1], 0);
            }
            else {
                addConnection(graph, boxToNode[b1], boxToNode[b2]);
            }
        }
        else {
            log_debug("Edge %d is taken.\n", e);
        }
    }
}

static BlissGraph * scGraphToBlissGraph(const SCGraph * scGraph) {
    BlissGraph * bGraph = bliss_new(0);

    // Color 0 differently to the other nodes since it's special.
    static unsigned int zeroColour = 0;
    static unsigned int othersColour = 1;

    bliss_add_vertex(bGraph, zeroColour);
    for (short i=1; i < scGraph->numNodes; i++) {
        bliss_add_vertex(bGraph, othersColour);
    }

    short allArcsBuf1[scGraph->numArcs];
    short allArcsBuf2[scGraph->numArcs];
    short numArcs = getAllArcs(scGraph, allArcsBuf1, allArcsBuf2);

    for (short i=0; i < numArcs; i++) {
        bliss_add_edge(bGraph, allArcsBuf1[i], allArcsBuf2[i]);
    }

    return bGraph;
}

void newAdjLists(SCGraph * graph) {
    graph->adjLists = (AdjList *)malloc(graph->numNodes * sizeof(AdjList));

    for(short i=0; i < graph->numNodes; i++) {
        graph->adjLists[i].length = 0;
        graph->adjLists[i].head = NULL;
    }
}

void freeAdjLists(SCGraph * graph) {
    for(short i=0; i < graph->numNodes; i++) {
        AdjList * al = &(graph->adjLists[i]);

        AdjListNode * node = al->head;
        while (node != NULL) {
            AdjListNode * nextNode = node->next;
            free(node);
            node = nextNode;
        }
    }

    free(graph->adjLists);
}

void copySCGraph(SCGraph * destGraph, const SCGraph * srcGraph) {
    // Will also malloc new adjLists in destGraph, so careful for memory leaks.
    
    destGraph->numNodes = srcGraph->numNodes;
    destGraph->numArcs = 0; // will be incremented when arcs are added

    newAdjLists(destGraph);

    for (short i=0; i < srcGraph->numNodes; i++) {
        destGraph->nodeToBox[i] = srcGraph->nodeToBox[i];
    }

    short allArcsBuf1[NUM_EDGES];
    short allArcsBuf2[NUM_EDGES];
    assert(getAllArcs(srcGraph, allArcsBuf1, allArcsBuf2) == srcGraph->numArcs);

    for(short i=0; i < srcGraph->numArcs; i++) {
        addConnection(destGraph, allArcsBuf1[i], allArcsBuf2[i]);
    }
}

static void printSCGraph(const SCGraph * graph) {
    for(short i=0; i < graph->numNodes; i++) {
        printf("%d -> ", i);
        for(short j=0; j < graph->numNodes; j++) {
            short numConnections = getNumConnectionsBetween(graph, i, j);

            for (short k=0; k < numConnections; k++) {
                printf("%d ", j);
            }
        }
        printf("\n");
    }
}

static AdjListNode * addAdjListNode(AdjList * list, short dest) {
    AdjListNode * newNode;

    if (list->head == NULL) {
        list->head = (AdjListNode *)malloc(sizeof(AdjListNode));
        newNode = list->head;
    }
    else {
        AdjListNode * connectedNode = list->head;
        while(connectedNode->next != NULL) {
            connectedNode = connectedNode->next;
        }

        connectedNode->next = (AdjListNode *)malloc(sizeof(AdjListNode));
        newNode = connectedNode->next;
    }

    newNode->dest = dest;
    newNode->next = NULL;

    list->length++;

    return newNode;
}

static void removeAdjListNode(AdjList * list, short dest) {
    if (list->head == NULL)
        return;
    else if (list->head->dest == dest) {
        AdjListNode * next = list->head->next;
        free(list->head);
        list->head = next;
        list->length = 0;
        return;
    }
    else {
        // Try and find a node which has the target dest.
        AdjListNode * prevNode = list->head;
        AdjListNode * currentNode = prevNode->next;
        
        while(currentNode->dest != dest) {
            prevNode = currentNode;
            currentNode = currentNode->next;

            // There isn't one so return
            if (currentNode == NULL)
                return;
        }

        // Else remove the link
        prevNode->next = currentNode->next;
        free(currentNode); 
        list->length--;
    }
}

static void addConnection(SCGraph * graph, short node1, short node2) {
    //log_debug("addConnection: Connecting %d and %d.\n", node1, node2);
    AdjList * node1List = &(graph->adjLists[node1]);
    AdjList * node2List = &(graph->adjLists[node2]);

    addAdjListNode(node1List, node2);
    addAdjListNode(node2List, node1);

    graph->numArcs++;
}


static void removeConnection(SCGraph * graph, short node1, short node2) {
    log_debug("removeConnection: Disconnecting %d and %d.\n", node1, node2);
    AdjList * node1List = &(graph->adjLists[node1]);
    AdjList * node2List = &(graph->adjLists[node2]);

    removeAdjListNode(node1List, node2);
    removeAdjListNode(node2List, node1);

    graph->numArcs--;
}

static short getNumConnectionsBetween(const SCGraph * graph, short node1, short node2) {
    short numConnections = 0;
    AdjList * node1List = &(graph->adjLists[node1]);

    AdjListNode * connectedNode = node1List->head;
    while (connectedNode != NULL) {
        if (connectedNode->dest == node2)
            numConnections++;

        connectedNode = connectedNode->next;
    }

    //log_debug("getNumConnectionsBetween: There are %d connections between %d and %d.\n", numConnections, node1, node2);

    return numConnections;
}

static bool areNodesConnected(const SCGraph * graph, short node1, short node2) {
    return getNumConnectionsBetween(graph, node1, node2) > 0;
}

static short getConnectedNodes(const SCGraph * graph, short node, short * nodeBuffer) {
    //log_debug("getConnectedNodes: Starting for node %d.\n", node);

    short numConnectedNodes = 0;

    AdjList * nodeList = &(graph->adjLists[node]);
    AdjListNode * connectedNode = nodeList->head;
    while(connectedNode != NULL) {
        //log_debug("getConnectedNodes: Adding %d.\n", connectedNode->dest);
        nodeBuffer[numConnectedNodes++] = connectedNode->dest;
        connectedNode = connectedNode->next;
    }

    return numConnectedNodes;
}

static short getNodeValency(const SCGraph * graph, short node) {
    return graph->adjLists[node].length;
}

static short findNodesWithValency(const SCGraph * graph, short valency, short * nodeBuf) {
    short numFound = 0;

    for (short i=1; i < graph->numNodes; i++) {
        if (getNodeValency(graph, i) == valency) {
            nodeBuf[numFound++] = i;
        }
    }

    return numFound;
}

static short getAllArcs(const SCGraph * graph, short * nodeBuf1, short * nodeBuf2) {
    // For each node pair e.g. (1,2), populate nodeBuf1 with the first element of the pair
    // and nodeBuf2 with the second element of the pair.
    // Returns the number of pairs.
    short numArcs = 0;
    
    for (short node=0; node < graph->numNodes; node++) {
        short neighbours[NEIGHBOUR_MAX]; // 0 could be connected to many neighbours
        short numNeighbours = getConnectedNodes(graph, node, neighbours);

        for (short i=0; i < numNeighbours; i++) {
            short neighbour = neighbours[i];

            if (neighbour > node) { // don't re-add duplicate pairs e.g. (3,4), (4,3)
                nodeBuf1[numArcs] = node;
                nodeBuf2[numArcs] = neighbour;
                numArcs++;
            }
        }
    }

    log_debug("numArcs: %d, scGraph->numArcs: %d\n", numArcs, graph->numArcs);
    assert(numArcs == graph->numArcs);

    return numArcs;
}

static short getSubGraphs(const SCGraph * superGraph, SCGraph *subGraphBuffer) {
    // Returns the number of sub-graphs found.
    short label = 0;
    short numAlreadyLabelled = 1; // let 0 be labelled
    BTree * alreadyLabelled = newBTree(0);
    short nodeToLabel[superGraph->numNodes];
    nodeToLabel[0] = 0; // node 0 (the imaginary node) is labelled with the unique label 0

    short currentLabelStack[NUM_BOXES];
    short currentLabelStackHead = -1;

    while (numAlreadyLabelled != superGraph->numNodes) {
        log_debug("getSubGraphs: Not all nodes have been labelled...\n");

        if (currentLabelStackHead == -1) { // the stack is empty
            label++;
            log_debug("Stack is empty. Increased label to %d. Searching for an unlabelled node...\n", label);

            // find an unlabelled node and push it
            for(short node=1; node < superGraph->numNodes; node++) {
                log_debug("getSubGraphs: considering node %d\n", node);

                if(!doesBTreeContain(alreadyLabelled, node)) {
                    log_debug("getSubGraphs: node is unlabelled. Pushing it to the stack.\n");
                    nodeToLabel[node] = label;
                    insertBTree(alreadyLabelled, node);
                    numAlreadyLabelled++;

                    currentLabelStack[++currentLabelStackHead] = node;
                    break;
                }
                else {
                    log_debug("getSubGraphs: node is already labelled.");
                }
            }
        }
        else {
            // Pop a node off the stack
            log_debug("Stack head at %d. Popping one...\n", currentLabelStackHead);
            short node = currentLabelStack[currentLabelStackHead--];

            // Infect it's neighbours
            short neighbours[NEIGHBOUR_MAX];
            short numNeighbours = getConnectedNodes(superGraph, node, neighbours);

            log_debug("numNeighbours: %d\n", numNeighbours);

            for(short i=0; i < numNeighbours; i++) {
                short neighbour = neighbours[i];
                log_debug("Considering neighbour: %d\n", neighbour);

                if(!doesBTreeContain(alreadyLabelled, neighbour)) {
                    numAlreadyLabelled++;
                    nodeToLabel[neighbour] = label;
                    insertBTree(alreadyLabelled, neighbour);

                    currentLabelStack[++currentLabelStackHead] = neighbour;
                    log_debug("Labelled neighbour with %d, numAlreadyLabelled: %d, nodeToLabel[%d] = %d, currentLabelStack[%d] = %d\n", label, numAlreadyLabelled, neighbour, label, currentLabelStackHead-1, neighbour);
                }
                else {
                    log_debug("Neighbour is already labelled.\n");
                }
            }
        }
    }
    log_debug("getSubGraphs: All nodes have been labelled!\n");

    short labelMax = label;
    log_debug("labelMax: %d\n", labelMax);

    for(short l=1; l <= labelMax; l++) {
        log_debug("SubGraph %d:\n", l);

        SCGraph * subGraph = &(subGraphBuffer[l-1]);
        subGraph->numArcs = 0;
        subGraph->numNodes = 0;
        subGraph->nodeToBox[0] = NO_BOX;

        // Iterate over all the nodes in the superGraph and partition those that are labelled with 'l'
        short subToSup[NUM_BOXES]; // the node indexed e.g. 4 in the superGraph might be indexed 1 in the subGraph. So this maps the two.
        subToSup[0] = 0;
        short subNodeCounter = 1;
        for(short superN=1; superN < superGraph->numNodes; superN++) {
            if (l == nodeToLabel[superN]) {
                log_debug("Box %d -> super node %d -> sub node %d\n", superGraph->nodeToBox[superN], superN, subNodeCounter);
                subGraph->nodeToBox[subNodeCounter] = superGraph->nodeToBox[superN];
                subToSup[subNodeCounter] = superN;
                subNodeCounter++;
            }
        }

        subGraph->numNodes = subNodeCounter;

        newAdjLists(subGraph);

        log_debug("subGraph->numNodes = %d\n", subNodeCounter);

        // Now iterate over the nodes in the partition and create the arcs in the subgraph.
        log_debug("Filling in arcs...\n");
        for(short subN1=0; subN1 < subNodeCounter; subN1++) {
            // initialize subN2 to subN1 so we don't re-add arcs for pairs e.g. (3,4) (4,3)
            for (short subN2=subN1; subN2 < subNodeCounter; subN2++) {
                short supN1 = subToSup[subN1];
                short supN2 = subToSup[subN2];

                log_debug("Considering sub nodes %d and %d. (super nodes %d and %d).\n", subN1, subN2, supN1, supN2);
                short numConnections = getNumConnectionsBetween(superGraph, supN1, supN2);
                log_debug("They are connected %d times.\n", numConnections);
                for (short i=0; i < numConnections; i++) {
                    addConnection(subGraph, subN1, subN2);
                }
            }
        } // end iterating subN1
    } // end iterating labels

    return labelMax;
}

static bool isGraphClosedChain(const SCGraph * graph) {
    // Closed chains do not connect to 0

    if (graph->numNodes - 2 != graph->numArcs || getNodeValency(graph, 0) > 0)
        return false;
    else {
        // There should be 2 1-nodes and the rest should be 2-nodes
        short v1Count = 0;

        for (short i=1; i < graph->numNodes; i++) {
            short valency = getNodeValency(graph, i);
            if (valency > 2)
                return false;
            else if (valency == 1)
                v1Count++;
        }

        if (v1Count == 2)
            return true;
        else
            return false;
    }
}

short getUrgentMoves(const SCGraph * graph, Edge * movesBuf) {
    bool foundMoves = false; // or just one move
    short numMoves = 0;

    short v1Nodes[NUM_BOXES];
    short numV1Nodes = findNodesWithValency(graph, 1, v1Nodes);

    //printSCGraph(graph);
    log_debug("getUrgentMoves: Doing quick pass for 1-nodes connected to joints (there are %d 1-nodes).\n", numV1Nodes);
    for (short i=0; i < numV1Nodes; i++) {
        short v1Node = v1Nodes[i];
        if (v1Node == 0) // avoid considering the imaginary node
            continue;

        log_debug("getUrgentMoves: Considering node %d.\n", v1Node);

        short neighbours[1];
        assert(getConnectedNodes(graph, v1Node, neighbours) == 1);

        short neighbour = neighbours[0];
        short neighbourValency = getNodeValency(graph, neighbour);
        log_debug("getUrgentMoves: Neighbour %d valency is %d.\n", neighbour, neighbourValency);
        // If neighbour is the imaginary node we should always take it.
        if (neighbour == 0 || neighbourValency >= 3) {
            foundMoves = true;
            movesBuf[numMoves++] = boxPairToEdge(graph->nodeToBox[v1Node], graph->nodeToBox[neighbour]);
            break;
        }
    }

    if(!foundMoves) {
        log_debug("getUrgentMoves: No 1-node cakes found... Proceeding with deeper search.\n");
        if(isGraphClosedChain(graph)) {
            short chainLength = graph->numNodes - 1;
            log_debug("getUrgentMoves: Closed chain detected of length %d.\n", chainLength);

            if (chainLength == 2) {
                foundMoves = true;
                movesBuf[numMoves++] = boxPairToEdge(graph->nodeToBox[1], graph->nodeToBox[2]);
            }
            else if (chainLength == 3) {
                /* Either move is fine but consider this:
                     . _ .
                     | 1 |
                 . _ .   .
                 | 2   3 |
                 . _ . _ .

                 Choosing nodes 1 and 2 would lead to an error because they aren't connected. But either of the other pairs are fine.
                 */
                short node = 1;
                short neighbours[2];
                getConnectedNodes(graph, node, neighbours); 
                short neighbour = neighbours[0];

                foundMoves = true;
                movesBuf[numMoves++] = boxPairToEdge(graph->nodeToBox[node], graph->nodeToBox[neighbour]);
            }
            else if (chainLength == 4) {
                short nodeBuffer[2];
                short neighbourBuffer[2];

                // a) Capture a 1 node and form a 3-chain
                assert(findNodesWithValency(graph, 1, nodeBuffer) == 2);
                short v1Node = nodeBuffer[0];

                assert(getConnectedNodes(graph, v1Node, neighbourBuffer) == 1);
                short v2Neighbour = neighbourBuffer[0];

                movesBuf[numMoves++] = boxPairToEdge(graph->nodeToBox[v1Node], graph->nodeToBox[v2Neighbour]);

                // b) Sacrifice down the middle.
                assert(findNodesWithValency(graph, 2, nodeBuffer) == 2);
                short v2Node = nodeBuffer[0];

                assert(getConnectedNodes(graph, v2Node, neighbourBuffer) == 2);
                if (getNodeValency(graph, neighbourBuffer[0]) == 2)
                    v2Neighbour = neighbourBuffer[0];
                else
                    v2Neighbour = neighbourBuffer[1];

                movesBuf[numMoves++] = boxPairToEdge(graph->nodeToBox[v2Node], graph->nodeToBox[v2Neighbour]);
            }
            else { // chain length is 5 or more, so take a box
                short nodeBuffer[2];
                short neighbourBuffer[1];

                assert(findNodesWithValency(graph, 1, nodeBuffer) == 2);
                short v1Node = nodeBuffer[0];

                assert(getConnectedNodes(graph, v1Node, neighbourBuffer) == 1);
                short v2Neighbour = neighbourBuffer[0];

                foundMoves = true;
                movesBuf[numMoves++] = boxPairToEdge(graph->nodeToBox[v1Node], graph->nodeToBox[v2Neighbour]);
            }
        }
        else { // look for open chains
            short joints[NUM_BOXES];
            joints[0] = 0; // joints are nodes with valency 3 or 4
            short numJoints = 1;
            BTree * visitedNodes = newBTree(0);

            for(short i=1; i < graph->numNodes; i++) {
                short valency = getNodeValency(graph, i);
                if (valency == 3 || valency == 4) {
                    joints[numJoints++] = i;
                    insertBTree(visitedNodes, i);
                }
            }

            // Iterate through the joints and try to make chains outward from them
            for(short i=0; i < numJoints; i++) {
                short jointNode = joints[i];

                short jointNeighbours[NEIGHBOUR_MAX];
                short numJointNeighbours = getConnectedNodes(graph, jointNode, jointNeighbours); 
                log_debug("Considering joint %d which has %d neighbours.\n", jointNode, numJointNeighbours);

                // Only consider neighbours of valency 2
                // (We already checked for neighbours with valency 1 above)
                for(short j=0; j < numJointNeighbours; j++) {
                    short jointNeighbour = jointNeighbours[j];

                    if (doesBTreeContain(visitedNodes, jointNeighbour))
                        continue;
                    else
                        insertBTree(visitedNodes, jointNeighbour);

                    log_debug("Considering joint neighbour %d with valency %d.\n", jointNeighbour, getNodeValency(graph, jointNeighbour));

                    if (!(getNodeValency(graph, jointNeighbour) == 2))
                        continue;

                    log_debug("Joint neighbour %d has valency 2! Exploring further...\n", jointNeighbour);
                    short chainLength = 1;
                    short prevNeighbour = jointNode;
                    short currentNeighbour = jointNeighbour;
                    
                    // Keep going along the chain until we hit a node of valency 1 or >= 3
                    while(getNodeValency(graph, currentNeighbour) == 2) {
                        short chainNeighbours[2];
                        assert(getConnectedNodes(graph, currentNeighbour, chainNeighbours) == 2);
                        if (chainNeighbours[0] == prevNeighbour) {
                            prevNeighbour = currentNeighbour;
                            currentNeighbour = chainNeighbours[1];
                        }
                        else { // chainNeighbours[1] == prevNeighbour
                            prevNeighbour = currentNeighbour;
                            currentNeighbour = chainNeighbours[0];
                        }

                        if (doesBTreeContain(visitedNodes, currentNeighbour))
                            break;
                        else
                            insertBTree(visitedNodes, currentNeighbour);

                        chainLength++;
                    }

                    // If the last node seen has valency 1, it's an open chain.
                    if (getNodeValency(graph, currentNeighbour) == 1) {
                        foundMoves = true;

                        if (chainLength == 2) {
                            // a) Take a box
                            movesBuf[numMoves++] = boxPairToEdge(graph->nodeToBox[prevNeighbour], graph->nodeToBox[currentNeighbour]);

                            // b) Sacrifice with hard-hearted-handout
                            movesBuf[numMoves++] = boxPairToEdge(graph->nodeToBox[jointNode], graph->nodeToBox[jointNeighbour]);
                        }
                        else { // chainLength >= 2 so take a box
                            movesBuf[numMoves++] = boxPairToEdge(graph->nodeToBox[prevNeighbour], graph->nodeToBox[currentNeighbour]);
                        }

                        break;
                    }
                    else {
                        log_debug("Hit a node with a valency other than 1. Stopping search.\n");
                    }
                } // end iterating joint neighbours


                if (foundMoves)
                    break;
            } // end iterating joints

            freeBTree(visitedNodes);
        } // end looking for open chains
    } // end if(!foundMoves)

    log_debug("getUrgentMoves: Returning a list of %d urgent moves.\n", numMoves);
    return numMoves;
}

static unsigned int getGraphCanonicalHash(const SCGraph * graph) {
    // Useful for quickly findings isormorphisms in sets of graphs.
    
    BlissGraph * bGraph = scGraphToBlissGraph(graph);

    BlissGraph * bGraphCanonical = bliss_permute(bGraph, bliss_find_canonical_labeling(bGraph, NULL, NULL, NULL));

    unsigned int hash = bliss_hash(bGraphCanonical);

    bliss_release(bGraph);
    bliss_release(bGraphCanonical);

    return hash;
}


static short getNonIsomorphicMoves(const SCGraph * graph, Edge * movesBuf) {
    // Return all moves which don't lead to isomorphic positions.
    log_debug("getNonIsomorphicMoves: Running for:\n");
    //printSCGraph(graph);

    short numMoves = 0;

    // getAllArcs populates two buffers with the two elements of each node pair defining an arc
    short allArcsBuf1[graph->numArcs];
    short allArcsBuf2[graph->numArcs];
    assert(getAllArcs(graph, allArcsBuf1, allArcsBuf2) == graph->numArcs);
    
    // Only unique graphs will be added to this set
    SCGraph childGraphs[graph->numArcs];
    unsigned int childGraphsHashes[graph->numArcs];
    short numChildGraphs = 0;

    for(short i=0; i < graph->numArcs; i++) {
        short node1 = allArcsBuf1[i];
        short node2 = allArcsBuf2[i];
        //log_debug("getNonIsomorphicMoves: Considering the arc between nodes %d and %d.\n", node1, node2);

        SCGraph childGraph;
        copySCGraph(&childGraph, graph);
        removeConnection(&childGraph, node1, node2);

        unsigned int childGraphHash = getGraphCanonicalHash(&childGraph);

        //log_debug("getNonIsomorphicMoves: childGraphHash = %u\n", childGraphHash);

        // Iterate through already-seen graphs
        bool isomorphic = false;
        for (short j=0; j < numChildGraphs; j++) {
            SCGraph * otherGraph = &childGraphs[j];

            if (childGraph.numNodes == otherGraph->numNodes &&
                    childGraph.numArcs == otherGraph->numArcs &&
                    childGraphHash == childGraphsHashes[j]) {
                log_debug("getNonIsomorphicMoves: Graphs are isomorphic!\n");
                isomorphic = true;
                break;
            }
        }

        if (!isomorphic) {
            Edge move = boxPairToEdge(graph->nodeToBox[node1], graph->nodeToBox[node2]);
            movesBuf[numMoves++] = move;
            childGraphsHashes[numChildGraphs] = childGraphHash;
            childGraphs[numChildGraphs++] = childGraph;

            //log_debug("getNonIsomorphicMoves: Move %d leads to an unseen position. Adding it.\n", move);
        }
    }

    // Free adjacency matrices for all created child graphs
    for(short i=0; i < numChildGraphs; i++) {
        freeAdjLists(&childGraphs[i]);
    }

    assert(numMoves != 0);
    return numMoves;
} 

short getGraphsPotentialMoves(const SCGraph * graph, Edge * potentialMoves) {
    SCGraph subGraphs[SUB_GRAPH_MAX];
    short numSubGraphs = getSubGraphs(graph, subGraphs);
    log_debug("getGraphsPotentialMoves: %d subgraphs found for given graph.\n", numSubGraphs);
    short numMoves = 0;

    bool foundUrgentMoves = false;
    // First check for urgent moves. If a subgraph has some, return them.
    for(short i=0; i < numSubGraphs; i++) {
        numMoves = getUrgentMoves(&subGraphs[i], potentialMoves);

        if (numMoves > 0) {
            foundUrgentMoves = true;
            break;
        }
    }

    if (!foundUrgentMoves) {
        // Else return all non urgent moves
        for(short i=0; i < numSubGraphs; i++) {
            numMoves += getNonIsomorphicMoves(&subGraphs[i], &(potentialMoves[numMoves]));
            freeAdjLists(&subGraphs[i]);
        }
    }

    log_debug("getGraphsPotentialMoves: Returning %d moves. urgent=%d\n", numMoves, foundUrgentMoves);

    return numMoves;
}

void runGraphsTests() {
    log_log("RUNNING GRAPHS TESTS\n");

    UnscoredState state;
    SCGraph graph;
    short potentialMoves[NUM_EDGES];
    short numPotentialMoves = 0;
    short nodeBuffer[NUM_BOXES + 1];

    SCGraph subGraphs[SUB_GRAPH_MAX];
    short numSubGraphs = 0;

    // Full graph:
    log_log("Testing full 28-node graph...\n");
    stringToUnscoredState(&state, "000000000000000000000000000000000000000000000000000000000000000000000000");

    log_debug("unscoredStateToSCGraph should behave correctly.\n");
    unscoredStateToSCGraph(&graph, &state);

    //printSCGraph(&graph);

    log_debug("It should have the right number of nodes.\n");
    assert(graph.numNodes == 28 + 1); // remember that NO_BOX is registered as a node

    log_debug("It should have the right number of arcs.\n");
    assert(graph.numArcs == 72);

    log_debug("nodeToBox should map correctly.\n");
    for(short i=1; i < graph.numNodes; i++) {
        // This is just a unique feature of the graph for the empty board
        assert(graph.nodeToBox[i] == i-1);
    }

    log_debug("All nodes should have valency 4.\n");
    short numNodesWithValency4 = findNodesWithValency(&graph, 4, nodeBuffer);
    log_debug("Found %d nodes with valency 4...\n", numNodesWithValency4);
    assert(numNodesWithValency4 == 28);

    log_log("Full 28-node graph passed!\n\n");

    log_log("Testing copySCGraph.\n");
    log_debug("It should exactly copy the 28-node graph into a newly declared graph.\n");
    SCGraph newGraph;
    copySCGraph(&newGraph, &graph);
    assert(newGraph.numNodes == graph.numNodes);
    assert(newGraph.numArcs == graph.numArcs);
    for(short i=0; i < graph.numNodes; i++) {
        assert(newGraph.nodeToBox[i] == graph.nodeToBox[i]);

        for(short j=0; j < graph.numNodes; j++) {
            log_debug("Testing i=%d, j=%d\n", i, j);
            assert(getNumConnectionsBetween(&graph, i, j) == getNumConnectionsBetween(&newGraph, i, j));
        }
    }
    freeAdjLists(&newGraph);
    freeAdjLists(&graph);

    // Graph with only a 2x2 square left
    log_log("Testing with a board with only a 2x2 square left...\n");
    stringToUnscoredState(&state, "111111111111011111110011111110111111111111111111111111111111111111111111");
    log_debug("unscoredStateToSCGraph should behave correctly.\n");
    unscoredStateToSCGraph(&graph, &state);

    printSCGraph(&graph);

    log_debug("It should have the right number of nodes.\n");
    assert(graph.numNodes = 4 + 1);

    log_debug("It should have the right number of arcs.\n");
    assert(graph.numArcs = 4);

    log_debug("nodeToBox should map correctly.\n");
    assert(graph.nodeToBox[0] == NO_BOX);
    assert(graph.nodeToBox[1] == 3);
    assert(graph.nodeToBox[2] == 4);
    assert(graph.nodeToBox[3] == 11);
    assert(graph.nodeToBox[4] == 12);

    log_debug("areNodesConnected should behave correctly.\n");
    assert(areNodesConnected(&graph,1,2));
    assert(areNodesConnected(&graph,1,3));
    assert(areNodesConnected(&graph,1,4) == false);

    freeAdjLists(&graph);

    log_log("4 boxes left graph passed!\n\n");

    /* Graph with isolated node:
      . _ .
    0   1 |
      . _ .
    */

    log_log("Testing with isolated node graph...\n");
    graph.numNodes = 2;
    graph.numArcs = 1;

    newAdjLists(&graph);
    graph.nodeToBox[1] = 1;
    addConnection(&graph, 0, 1);

    printSCGraph(&graph);

    log_debug("getUrgentMoves should return the 1 possible edge...\n");
    numPotentialMoves = getUrgentMoves(&graph, potentialMoves);
    assert(numPotentialMoves == 1);
    assert(potentialMoves[0] == boxPairToEdge(NO_BOX,1));

    freeAdjLists(&graph);

    log_log("Isolated node graph passed!\n\n");

    /* Graph with isolated 2-node as a subgraph:
    */

    log_log("Testing with isolated 2-node subgraph...\n");
    stringToUnscoredState(&state, "000000000000000000000000000000000000000000000000000000000001000100000000");
    unscoredStateToSCGraph(&graph, &state);
    printSCGraph(&graph);

    numSubGraphs = getSubGraphs(&graph, subGraphs);
    assert(numSubGraphs == 2);

    log_debug("getUrgentMoves should not return anything for the subgraph...\n");
    numPotentialMoves = getUrgentMoves(&subGraphs[1], potentialMoves);
    assert(numPotentialMoves == 0);

    log_debug("getNonIsomorphicMoves should return one move for the subgraph...\n");
    numPotentialMoves = getNonIsomorphicMoves(&subGraphs[1], potentialMoves);
    assert(numPotentialMoves == 1);

    freeAdjLists(&graph);

    log_log("Isolated 2-node subgraph passed!\n\n");

    /* Graph with 2 boxes connecting to 0:
    . _ .
    | 1  
    .   .
    | 2
    . _ .
    */

    log_log("Testing with 2 box graph where both connect to 0...\n");
    graph.numNodes = 3;
    graph.numArcs = 0;

    newAdjLists(&graph);
    graph.nodeToBox[0] = NO_BOX;
    graph.nodeToBox[1] = 1;
    graph.nodeToBox[2] = 2;
    addConnection(&graph, 1, 0);
    addConnection(&graph, 1, 2);
    addConnection(&graph, 2, 0);

    printSCGraph(&graph);

    log_debug("addConnection should correctly increase the number of arcs.\n");
    assert(graph.numArcs == 3);

    log_debug("getUrgentMoves should not return any moves.\n");
    numPotentialMoves = getUrgentMoves(&graph, potentialMoves);
    assert(numPotentialMoves == 0);

    log_debug("copySCGraph should work correctly for this graph.\n");
    SCGraph childGraph;
    copySCGraph(&childGraph, &graph);

    log_debug("Before removing connection:\n");
    printSCGraph(&childGraph);
    assert(areNodesConnected(&childGraph, 1, 0));
    assert(areNodesConnected(&childGraph, 1, 2));
    assert(areNodesConnected(&childGraph, 2, 0));
    
    removeConnection(&childGraph, 1, 0);
    log_debug("After removing connection:\n");
    printSCGraph(&childGraph);

    assert(areNodesConnected(&childGraph, 1, 0) == false);
    assert(areNodesConnected(&childGraph, 1, 2));
    assert(areNodesConnected(&childGraph, 2, 0));

    freeAdjLists(&childGraph);

    log_debug("getNonIsomorphicMoves should return the 2 sensible moves.\n");
    numPotentialMoves = getNonIsomorphicMoves(&graph, potentialMoves);
    assert(numPotentialMoves == 2);

    freeAdjLists(&graph);

    log_log("2 box graph where both connect to 0 passed!\n\n");
    
    /* Graph with 2-chain:
    . _ . _ .
    | 1   2 |
    . _ . _ .
    */
    log_log("Testing with a 2-chain graph...\n");
    graph.numNodes = 3;
    graph.numArcs = 0;

    newAdjLists(&graph);
    addConnection(&graph, 1, 2);

    printSCGraph(&graph);

    assert(isGraphClosedChain(&graph));

    freeAdjLists(&graph);

    log_log("2-chain graph passed!\n\n");

    /* Graph with open 2-chain:
    . _ . _ .
    | 1   2  
    . _ . _ .
    */
    log_log("Testing with an open 2-chain graph...\n");
    graph.numNodes = 3;
    graph.numArcs = 0;

    newAdjLists(&graph);
    addConnection(&graph, 1, 2);
    addConnection(&graph, 2, 0);
    graph.nodeToBox[0] = NO_BOX;
    graph.nodeToBox[1] = 1;
    graph.nodeToBox[2] = 2;

    printSCGraph(&graph);

    log_debug("isGraphClosedChain should return false.\n");
    assert(isGraphClosedChain(&graph) == false);

    log_debug("getUrgentMoves should return the 2 sensible moves.\n");
    numPotentialMoves = getUrgentMoves(&graph, potentialMoves);
    assert(numPotentialMoves == 2);

    Edge move1 = boxPairToEdge(1,2);
    Edge move2 = boxPairToEdge(2,NO_BOX);

    assert((potentialMoves[0] == move1 && potentialMoves[1] == move2) ||
           (potentialMoves[1] == move1 && potentialMoves[0] == move2));

    freeAdjLists(&graph);

    log_log("Open 2-chain graph passed!\n\n");

    /* Graph with 4-chain:
    . _ . _ . _ . _ .
    | 1   2   3   4 |
    . _ . _ . _ . _ .
    */
    log_log("Testing with 4-chain graph...\n");
    graph.numNodes = 5;
    graph.numArcs = 0;

    newAdjLists(&graph);
    graph.nodeToBox[0] = NO_BOX;
    graph.nodeToBox[1] = 1;
    graph.nodeToBox[2] = 2;
    graph.nodeToBox[3] = 3;
    graph.nodeToBox[4] = 4;
    addConnection(&graph, 1, 2);
    addConnection(&graph, 2, 3);
    addConnection(&graph, 3, 4);

    printSCGraph(&graph);

    log_debug("isGraphClosedChain should return true.\n");
    assert(isGraphClosedChain(&graph) == true);

    log_debug("getUrgentMoves should return the 2 sensible moves.\n");
    numPotentialMoves = getUrgentMoves(&graph, potentialMoves);
    assert(numPotentialMoves == 2);

    move1 = boxPairToEdge(1,2);
    move2 = boxPairToEdge(2,3);

    assert((potentialMoves[0] == move1 && potentialMoves[1] == move2) ||
           (potentialMoves[1] == move1 && potentialMoves[0] == move2));

    freeAdjLists(&graph);

    log_log("4-chain graph passed!\n\n");

    /* Graph with open 4-chain:
    . _ . _ . _ . _ .
    | 1   2   3   4  
    . _ . _ . _ . _ .
    */
    log_log("Testing with an open 4-chain graph...\n");
    graph.numNodes = 5;
    graph.numArcs = 0;

    newAdjLists(&graph);
    graph.nodeToBox[0] = NO_BOX;
    graph.nodeToBox[1] = 1;
    graph.nodeToBox[2] = 2;
    graph.nodeToBox[3] = 3;
    graph.nodeToBox[4] = 4;
    addConnection(&graph, 1, 2);
    addConnection(&graph, 2, 3);
    addConnection(&graph, 3, 4);
    addConnection(&graph, 4, 0);

    printSCGraph(&graph);

    log_debug("isGraphClosedChain should return false.\n");
    assert(isGraphClosedChain(&graph) == false);

    log_debug("getUrgentMoves should return the 1 urgent move.\n");
    numPotentialMoves = getUrgentMoves(&graph, potentialMoves);
    assert(numPotentialMoves == 1);

    assert(potentialMoves[0] == boxPairToEdge(1,2));

    freeAdjLists(&graph);

    log_log("Open 4-chain graph passed!\n\n");

    /* Graph with 5-chain:
    . _ . _ . _ . _ . _ .
    | 1   2   3   4   5 |
    . _ . _ . _ . _ . _ .
    */
    log_log("Testing with 5-chain graph...\n");
    graph.numNodes = 6;
    graph.numArcs = 0;

    newAdjLists(&graph);
    graph.nodeToBox[0] = NO_BOX;
    graph.nodeToBox[1] = 1;
    graph.nodeToBox[2] = 2;
    graph.nodeToBox[3] = 3;
    graph.nodeToBox[4] = 4;
    graph.nodeToBox[5] = 5;
    addConnection(&graph, 1, 2);
    addConnection(&graph, 2, 3);
    addConnection(&graph, 3, 4);
    addConnection(&graph, 4, 5);

    printSCGraph(&graph);

    log_debug("isGraphClosedChain should return true.\n");
    assert(isGraphClosedChain(&graph) == true);

    log_debug("getUrgentMoves should return 1 cake move.\n");
    numPotentialMoves = getUrgentMoves(&graph, potentialMoves);
    assert(numPotentialMoves == 1);

    assert(potentialMoves[0] == boxPairToEdge(1,2));

    freeAdjLists(&graph);

    log_log("5-chain graph passed!\n\n");

    /* Trivial 2x3 graph:
    . _ . _ . _ .
    | 1 | 2   3 |
    .   . _ . _ .
    | 4   5   6 |
    . _ . _ . _ .
    */
    log_log("Testing with a trivial 2x3 graph...\n");

    graph.numNodes = 7;
    graph.numArcs = 0;
    
    newAdjLists(&graph);

    graph.nodeToBox[0] = NO_BOX;
    graph.nodeToBox[1] = 1;
    graph.nodeToBox[2] = 2;
    graph.nodeToBox[3] = 3;
    graph.nodeToBox[4] = 4;
    graph.nodeToBox[5] = 5;
    graph.nodeToBox[6] = 6;

    // Group 1
    addConnection(&graph, 1, 4);
    addConnection(&graph, 4, 5);
    addConnection(&graph, 5, 6);

    // Group 2
    addConnection(&graph, 2, 3);

    printSCGraph(&graph);

    log_debug("areNodesConnected should behave properly.\n");
    assert(areNodesConnected(&graph, 1, 4));
    assert(areNodesConnected(&graph, 2, 3));
    assert(areNodesConnected(&graph, 4, 5));
    assert(areNodesConnected(&graph, 5, 6));

    log_debug("getSubGraphs should return the right subGraphs.\n");
    numSubGraphs = getSubGraphs(&graph, subGraphs);
    log_debug("numSubGraphs: %d\n", numSubGraphs);
    assert(numSubGraphs == 2);

    assert(subGraphs[0].numNodes == 5);
    assert(subGraphs[0].numArcs == 3);

    assert(subGraphs[1].numNodes == 3);
    assert(subGraphs[1].numArcs == 1);

    freeAdjLists(&graph);

    log_log("2x3 graph passed!\n\n");

    /* A more interesting position featuring 2 cakes, a long chain and some 0 connections.
    . _ .   . _ .
    | 1   2 | 3  
    .   . _ . _ .
    | 4 | 5   6 |
    .   . _ . _ .
    | 7   8   9 |
    . _ . _ . _ .
    */
    log_log("Testing with a more complex 3x3 graph.\n");

    graph.numNodes = 10;
    graph.numArcs = 0;

    newAdjLists(&graph);

    graph.nodeToBox[0] = NO_BOX;
    graph.nodeToBox[1] = 1;
    graph.nodeToBox[2] = 2;
    graph.nodeToBox[3] = 3;
    graph.nodeToBox[4] = 4;
    graph.nodeToBox[5] = 5;
    graph.nodeToBox[6] = 6;
    graph.nodeToBox[7] = 7;
    graph.nodeToBox[8] = 8;
    graph.nodeToBox[9] = 9;
    
    addConnection(&graph, 0, 2);
    addConnection(&graph, 1, 2);
    addConnection(&graph, 1, 4);
    addConnection(&graph, 4, 7);
    addConnection(&graph, 7, 8);
    addConnection(&graph, 8, 9);
    addConnection(&graph, 3, 0);
    addConnection(&graph, 5, 6);

    printSCGraph(&graph);

    log_debug("getSubGraphs should return the right number of subgraphs.\n");
    numSubGraphs = getSubGraphs(&graph, subGraphs);
    assert(numSubGraphs == 3);

    log_debug("The first subgraph should be correct.\n");
    assert(subGraphs[0].numNodes == 7);
    assert(subGraphs[0].numArcs == 6);
    assert(areNodesConnected(&subGraphs[0], 0, 2) == true);
    assert(areNodesConnected(&subGraphs[0], 2, 1) == true);
    assert(areNodesConnected(&subGraphs[0], 1, 3) == true);
    assert(areNodesConnected(&subGraphs[0], 3, 4) == true);
    assert(areNodesConnected(&subGraphs[0], 4, 5) == true);

    log_debug("The second subgraph should be correct.\n");
    assert(subGraphs[1].numNodes == 2);
    assert(subGraphs[1].numArcs == 1);
    assert(areNodesConnected(&subGraphs[1], 0, 1) == true);

    log_debug("getUrgentMoves should return the only edge for the second subgraph (a 0->1 cake).\n");
    numPotentialMoves = getUrgentMoves(&subGraphs[1], potentialMoves);
    assert(numPotentialMoves == 1);
    assert(potentialMoves[0] == boxPairToEdge(3,NO_BOX));

    log_debug("The third subgraph should be correct.\n");
    assert(subGraphs[2].numNodes == 3);
    assert(subGraphs[2].numArcs == 1);
    assert(areNodesConnected(&subGraphs[2], 1, 2) == true);

    log_debug("getUrgentMoves should return the only edge for the third subgraph (a 2-chain).\n");
    numPotentialMoves = getUrgentMoves(&subGraphs[2], potentialMoves);
    assert(numPotentialMoves == 1);
    assert(potentialMoves[0] == boxPairToEdge(5,6));

    freeAdjLists(&graph);
    log_log("3x3 graph passed!\n\n");

    /* Complex 28 box position:
    •――•――•――•――•――•――•――•――•
    |1  2 |3  4  5  6 |7  8 |
    •――•――•――•――•――•――•  •  •
     9 |10 11 12 13 14|15 16|
    •――•――•  •――•――•――•――•――•
       |17|18|     |19 20|  
       •  •――•     •  •  •  
       |21|22|     |23|24|  
       •  •  •     •  •  •  
       |25|26|     |27 28|
       •  •  •     •――•――•
    */

    log_log("Testing with a complex 29 node graph.\n");

    stringToUnscoredState(&state, "111111111010001011111110001000010111011111111101010011111100001111010011");
    printUnscoredState(&state);
    unscoredStateToSCGraph(&graph, &state);

    printSCGraph(&graph);

    log_debug("unscoredStateToSCGraph should behave correctly.\n");
    assert(graph.numNodes == 29);
    assert(graph.numArcs == 25);

    assert(graph.nodeToBox[0] = NO_BOX);
    for(short i=1; i <= 28; i++) {
        assert(graph.nodeToBox[i] == i-1);
    }
    
    // Group 1
    assert(areNodesConnected(&graph, 1, 2));

    // Group 2
    assert(areNodesConnected(&graph, 3, 4));
    assert(areNodesConnected(&graph, 4, 5));
    assert(areNodesConnected(&graph, 5, 6));

    // Group 3
    assert(areNodesConnected(&graph, 7, 8));
    assert(areNodesConnected(&graph, 7, 15));
    assert(areNodesConnected(&graph, 16, 8));
    assert(areNodesConnected(&graph, 16, 15));

    // Group 4
    assert(areNodesConnected(&graph, 0, 9));

    // Group 5
    assert(areNodesConnected(&graph, 10, 11));
    assert(areNodesConnected(&graph, 11, 18));
    assert(areNodesConnected(&graph, 11, 12));
    assert(areNodesConnected(&graph, 12, 13));
    assert(areNodesConnected(&graph, 13, 14));

    // Group 6
    assert(areNodesConnected(&graph, 17, 21));
    assert(areNodesConnected(&graph, 21, 25));
    assert(areNodesConnected(&graph, 25, 0));

    // Group 7
    assert(areNodesConnected(&graph, 19, 20));
    assert(areNodesConnected(&graph, 20, 24));
    assert(areNodesConnected(&graph, 24, 28));
    assert(areNodesConnected(&graph, 28, 27));
    assert(areNodesConnected(&graph, 27, 23));
    assert(areNodesConnected(&graph, 23, 19));

    // Group 8
    assert(areNodesConnected(&graph, 22, 26));

    log_debug("getSubGraphs should return the right number of subgraphs.\n");
    numSubGraphs = getSubGraphs(&graph, subGraphs);
    assert(numSubGraphs == 8);

    // Subgraph 1
    log_log("Subgraph 1 should be correct.\n");
    assert(subGraphs[0].numNodes == 3);
    assert(subGraphs[0].numArcs == 1);
    assert(areNodesConnected(&subGraphs[0], 1, 2) == true);
    assert(areNodesConnected(&subGraphs[0], 0, 1) == false);
    assert(areNodesConnected(&subGraphs[0], 0, 2) == false);

    log_debug("It should be recognized as a closed chain of length 2.\n");
    assert(isGraphClosedChain(&subGraphs[0]));

    log_debug("getUrgentMoves should return the 1 urgent move.\n");
    numPotentialMoves = getUrgentMoves(&subGraphs[0], potentialMoves);
    assert(numPotentialMoves == 1);
    assert(potentialMoves[0] == boxPairToEdge(0,1));

    // Subgraph 2
    log_log("Subgraph 2 should be correct.\n");
    assert(subGraphs[1].numNodes == 5);
    assert(subGraphs[1].numArcs == 3);
    assert(areNodesConnected(&subGraphs[1], 1, 2) == true);
    assert(areNodesConnected(&subGraphs[1], 2, 3) == true);
    assert(areNodesConnected(&subGraphs[1], 3, 4) == true);
    assert(areNodesConnected(&subGraphs[1], 0, 1) == false);
    assert(areNodesConnected(&subGraphs[1], 0, 2) == false);
    assert(areNodesConnected(&subGraphs[1], 0, 3) == false);

    log_debug("It should be recognized as a closed chain of length 4.\n");
    assert(isGraphClosedChain(&subGraphs[1]));

    log_debug("getUrgentMoves should return the 2 urgent moves.\n");
    numPotentialMoves = getUrgentMoves(&subGraphs[1], potentialMoves);
    assert(numPotentialMoves == 2);
    assert(potentialMoves[0] == boxPairToEdge(2,3));
    assert(potentialMoves[1] == boxPairToEdge(3,4));

    // Subgraph 3
    log_log("Subgraph 3 should be correct.\n");
    assert(subGraphs[2].numNodes == 5);
    assert(subGraphs[2].numArcs == 4);
    assert(areNodesConnected(&subGraphs[2], 1, 2) == true);
    assert(areNodesConnected(&subGraphs[2], 1, 3) == true);
    assert(areNodesConnected(&subGraphs[2], 2, 4) == true);
    assert(areNodesConnected(&subGraphs[2], 3, 4) == true);

    log_debug("No urgent moves should be returned.\n");
    numPotentialMoves = getUrgentMoves(&subGraphs[2], potentialMoves);
    assert(numPotentialMoves == 0);

    log_debug("getNonIsomorphicMoves should return 1 move since all others are isomorphic.\n");
    numPotentialMoves = getNonIsomorphicMoves(&subGraphs[2], potentialMoves);
    assert(numPotentialMoves == 1);

    // Subgraph 4
    log_log("Subgraph 4 should be correct.\n");
    assert(subGraphs[3].numNodes == 2);
    assert(subGraphs[3].numArcs == 1);
    assert(areNodesConnected(&subGraphs[3], 0, 1) == true);

    log_debug("It should not be recognized as a closed chain.\n");
    assert(isGraphClosedChain(&subGraphs[3]) == false);

    log_debug("getUrgentMoves should return the 1 urgent move.\n");
    numPotentialMoves = getUrgentMoves(&subGraphs[3], potentialMoves);
    assert(numPotentialMoves == 1);
    assert(potentialMoves[0] == boxPairToEdge(NO_BOX, 8));

    // Subgraph 5
    log_log("Subgraph 5 should be correct.\n");
    assert(subGraphs[4].numNodes == 7);
    assert(subGraphs[4].numArcs == 5);
    assert(areNodesConnected(&subGraphs[4], 1, 2) == true);
    assert(areNodesConnected(&subGraphs[4], 2, 3) == true);
    assert(areNodesConnected(&subGraphs[4], 3, 4) == true);
    assert(areNodesConnected(&subGraphs[4], 4, 5) == true);
    assert(areNodesConnected(&subGraphs[4], 2, 6) == true);

    log_debug("getUrgentMoves should return just the 1st urgent move.\n");
    numPotentialMoves = getUrgentMoves(&subGraphs[4], potentialMoves);
    assert(numPotentialMoves == 1);
    assert(potentialMoves[0] == boxPairToEdge(9, 10));

    // Subgraph 6
    log_log("Subgraph 6 should be correct.\n");
    assert(subGraphs[5].numNodes == 4);
    assert(subGraphs[5].numArcs == 3);
    assert(areNodesConnected(&subGraphs[5], 1, 2) == true);
    assert(areNodesConnected(&subGraphs[5], 2, 3) == true);
    assert(areNodesConnected(&subGraphs[5], 3, 0) == true);

    log_debug("getUrgentMoves should recognize an open chain of length 3 and return the 1 urgent move.\n");
    numPotentialMoves = getUrgentMoves(&subGraphs[5], potentialMoves);
    assert(numPotentialMoves == 1);
    assert(potentialMoves[0] == boxPairToEdge(16, 20));

    // Subgraph 7
    log_log("Subgraph 7 should be correct.\n");
    assert(subGraphs[6].numNodes == 7);
    assert(subGraphs[6].numArcs == 6);
    assert(areNodesConnected(&subGraphs[6], 1, 2) == true);
    assert(areNodesConnected(&subGraphs[6], 2, 4) == true);
    assert(areNodesConnected(&subGraphs[6], 4, 6) == true);
    assert(areNodesConnected(&subGraphs[6], 6, 5) == true);
    assert(areNodesConnected(&subGraphs[6], 5, 3) == true);
    assert(areNodesConnected(&subGraphs[6], 3, 1) == true);

    log_debug("No urgent moves should be returned.\n");
    numPotentialMoves = getUrgentMoves(&subGraphs[6], potentialMoves);
    assert(numPotentialMoves == 0);

    // Subgraph 8
    log_log("Subgraph 8 should be correct.\n");
    assert(subGraphs[7].numNodes == 3);
    assert(subGraphs[7].numArcs == 2);
    assert(areNodesConnected(&subGraphs[7], 1, 2) == true);
    assert(areNodesConnected(&subGraphs[7], 2, 0) == true);

    log_debug("getUrgentMoves should recognize an open chain of length 2 and return the 2 urgent moves.\n");
    numPotentialMoves = getUrgentMoves(&subGraphs[7], potentialMoves);
    assert(numPotentialMoves == 2);

    move1 = boxPairToEdge(21,25);
    move2 = boxPairToEdge(25,NO_BOX);
    assert((potentialMoves[0] == move1 && potentialMoves[1] == move2) ||
           (potentialMoves[1] == move1 && potentialMoves[0] == move2));

    freeAdjLists(&graph);
    log_log("Complex 28 box graph passed!\n\n");

    log_log("GRAPHS TESTS COMPLETED\n\n");
}
