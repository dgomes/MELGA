#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#ifdef DAEMON
#define SYSLOG
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#endif

#ifdef SYSLOG
#include <syslog.h>
#endif


#ifdef DEBUG
#define DBG(...) \
            do { fprintf(stderr, ##__VA_ARGS__); } while (0)
#else
#define DBG(fmt, ...)
#endif

#ifndef SYSLOG
#define NOTICE(...) \
            do { fprintf(stdout, ANSI_COLOR_CYAN); fprintf(stdout, ##__VA_ARGS__); fprintf(stdout, ANSI_COLOR_RESET "\n"); } while (0)
#define ERR(...) \
            do { fprintf(stderr, ANSI_COLOR_RED); fprintf(stderr, ##__VA_ARGS__); fprintf(stderr, ANSI_COLOR_RESET "\n"); } while (0)
#define INFO(...) \
            do { fprintf(stdout, ##__VA_ARGS__); } while (0)
#else
#define NOTICE(...) \
            syslog(LOG_NOTICE, ##__VA_ARGS__);
#define ERR(...) \
            syslog(LOG_ERR, ##__VA_ARGS__);
#define INFO(...) \
            syslog(LOG_INFO, ##__VA_ARGS__);
#endif

char* strlaststr(const char *haystack, const char* needle);
void daemonize();

#endif
