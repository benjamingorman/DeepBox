#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include "util.h"

BTree * initBTree(int value) {
    BTree * bt = (BTree *)malloc(sizeof(BTree));
    bt->value = value;
    bt->left = NULL;
    bt->right = NULL;

    return bt;
}

void freeBTree(BTree * bt) {
    if (bt != NULL) {
        freeBTree(bt->left);
        freeBTree(bt->right);
        free(bt);
    }
}

BTree * insertBTree(BTree * bt, int value) {
    if (value == bt->value)
        return bt;
    else if (value < bt->value) {
        if (bt->left == NULL) {
            bt->left = initBTree(value);
            return bt->left;
        }
        else
            insertBTree(bt->left, value);
    }
    else { // value > bt->value
        if (bt->right == NULL) {
            bt->right = initBTree(value);
            return bt->right;
        }
        else
            insertBTree(bt->right, value);
    }

    return NULL;
}

bool doesBTreeContain(BTree * bt, int value) {
    if (bt->value == value)
        return true;
    else if (bt->left != NULL && value < bt->value)
        return doesBTreeContain(bt->left, value);
    else if (bt->right != NULL && value > bt->value)
        return doesBTreeContain(bt->right, value);
    else
        return false;
}

int max(int a, int b) {
    return a > b ? a : b;
}

int min(int a, int b) {
    return a < b ? a : b;
}

unsigned long long getTimeMillis() {
    struct timeval t;
    gettimeofday(&t, NULL);
    unsigned long long tMillis =
        (unsigned long long)(t.tv_sec) * 1000 +
        (unsigned long long)(t.tv_usec) / 1000;

    return tMillis;
}

int randomInRange(unsigned int min, unsigned int max) {
    // Credit: http://stackoverflow.com/questions/2509679/how-to-generate-a-random-number-from-within-a-range
    int r;

    const unsigned int range = 1 + max - min;
    const unsigned int buckets = RAND_MAX / range;
    const unsigned int limit = buckets * range;

    /* Create equal size buckets all in a row, then fire randomly towards
     * the buckets until you land in one of them. All buckets are equally
     * likely. If you land off the end of the line of buckets, try again. */
    do {
        r = rand();
    } while (r >= limit);

    return min + (r / buckets);
}



void runUtilTests() {
    log_log("RUNNING UTIL TESTS\n");

    log_log("Testing randomInRange...\n");
    log_debug("It should return 0 for interval 0, 0...\n");
    int r = randomInRange(0, 0);
    assert(r == 0);

    log_debug("It should return 0 or 1 for interval 1, 0...\n");
    r = randomInRange(0,1);
    assert(r == 0 || r == 1);

    log_log("Testing initBTree...\n");
    log_debug("It should initialize the values correctly.\n");
    BTree * btRoot = initBTree(3);
    assert(btRoot->value == 3);
    assert(btRoot->left == NULL);
    assert(btRoot->right == NULL);

    log_log("Testing insertBTree...\n");
    log_debug("It should correctly insert a child node with a value lower than the root node.\n");
    BTree * btRootLeftChild = insertBTree(btRoot, 1);
    assert(btRootLeftChild->value == 1);
    assert(btRootLeftChild->left == NULL);
    assert(btRootLeftChild->right == NULL);
    assert(btRoot->left == btRootLeftChild);

    log_debug("It should correctly insert a child node with a value higher than the root node.\n");
    BTree * btRootRightChild = insertBTree(btRoot, 5);
    assert(btRootRightChild->value == 5);
    assert(btRootRightChild->left == NULL);
    assert(btRootRightChild->right == NULL);
    assert(btRoot->right == btRootRightChild);

    log_log("Testing doesBTreeContain...\n");
    log_debug("It should return true if the value of the given tree is also the target search value.\n");
    assert(doesBTreeContain(btRoot, 3) == true);

    log_debug("It should return true if a lower value than the root value exists in the tree.\n");
    assert(doesBTreeContain(btRoot, btRootLeftChild->value) == true);

    log_debug("It should return true if a higher value than the root value exists in the tree.\n");
    assert(doesBTreeContain(btRoot, btRootRightChild->value) == true);

    log_debug("It should return false if a value that is not in the tree is searched for.\n");
    assert(doesBTreeContain(btRoot, 100) == false);

    log_log("UTIL TESTS COMPLETED\n\n");
}
