#ifndef UTIL_H
#define UTIL_H

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_LOG 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_ERROR 3

#define LOG_LEVEL LOG_LEVEL_LOG 

#define log_debug(...) \
    do { if (LOG_LEVEL <= LOG_LEVEL_DEBUG) fprintf(stdout, __VA_ARGS__); } while (0)
#define log_log(...) \
    do { if (LOG_LEVEL <= LOG_LEVEL_LOG) fprintf(stdout, __VA_ARGS__); } while (0)
#define log_warn(...) \
    do { if (LOG_LEVEL <= LOG_LEVEL_WARN) fprintf(stderr, __VA_ARGS__); } while (0)
#define log_error(...) \
    do { if (LOG_LEVEL <= LOG_LEVEL_ERROR) fprintf(stderr, __VA_ARGS__); } while (0)

unsigned long long getTimeMillis();
int randomInRange(unsigned int min, unsigned int max);
void runUtilTests();

#endif
