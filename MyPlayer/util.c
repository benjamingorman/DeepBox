#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include "util.h"

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

    log_log("Testing randomInRange returns 0 for interval 0, 0...\n");
    int r = randomInRange(0, 0);
    assert(r == 0);

    log_log("Testing randomInRange returns 0 or 1 for interval 0, 1...\n");
    r = randomInRange(0,1);
    assert(r == 0 || r == 1);

    log_log("UTIL TESTS COMPLETED\n");
}
