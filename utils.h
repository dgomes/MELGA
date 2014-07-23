#include <stdlib.h>
#include <syslog.h>
#include <string.h>

#ifndef DEBUG
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#endif

#ifdef DEBUG
#define DBG(...) \
            do { fprintf(stderr, ##__VA_ARGS__); } while (0)
#define NOTICE(...) \
            do { fprintf(stdout, "\x1B[36m"); fprintf(stdout, ##__VA_ARGS__); fprintf(stdout, "\x1B[0m\n"); } while (0)
#define ERR(...) \
            do { fprintf(stdout, "\x1B[31m"); fprintf(stdout, ##__VA_ARGS__); fprintf(stdout, "\x1B[0m\n"); } while (0)
#define INFO(...) \
            do { fprintf(stdout, "\x1B[32m"); fprintf(stdout, ##__VA_ARGS__); fprintf(stdout, "\x1B[0m\n"); } while (0)
#else
#define DBG(fmt, ...)
#define NOTICE(...) \
            syslog(LOG_NOTICE, ##__VA_ARGS__);
#define ERR(...) \
            syslog(LOG_ERR, ##__VA_ARGS__);
#define INFO(...) \
            syslog(LOG_INFO, ##__VA_ARGS__);
#endif

char* strlastsrt(const char *haystack, const char* needle);
void daemonize();
