#ifndef __HTTPD_LOGIT_H__
#define __HTTPD_LOGIT_H__

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"

extern FILE *logfp;

#if 1
#define logit_init() \
	do { \
        logfp = stdout; \
	} while(0);
#else
#define logit_init() \
	do { \
        logfp = stdout; \
		/* logfp = fopen(LOGFILE, "w+"); \ */ \
		if (logfp == NULL) { \
			fprintf(stderr, "ERROR: failed to init logger: fopen() failed for %s (%s)\n", \
			                                                   LOGFILE, strerror(errno)); \
			abort(); \
		} \
	} while(0);
#endif

#define logit_close() \
	do { \
		if (logfp != NULL) \
			fclose(logfp); \
	} while(0);

#define logit_enter()  logit("Enter")
#define logit_finish() logit("Finish")

#define logit_retval() \
	do { \
		if (retval != RETSXS) \
			logit("retval: %d", retval); \
	} while(0);

#define logit(...) \
	do { \
		fprintf(logfp, "DEBUG %d %17s:%-4d %25s() :: ", \
		        getpid(), __FILE__, __LINE__, __func__); \
		fprintf(logfp, __VA_ARGS__); \
		fprintf(logfp, "\n"); \
	} while(0);

#define httpd_error(...) \
	do { \
		fprintf(stderr, "ERROR %d %17s:%-4d %25s() :: ", \
		        getpid(), __FILE__, __LINE__, __func__); \
		fprintf(stderr, __VA_ARGS__); \
		fprintf(stderr, "\n"); \
	} while(0);

#define httpd_printf(...) \
	do { \
		fprintf(stdout, __VA_ARGS__); \
		fprintf(stdout, "\n"); \
	} while(0);

#endif
