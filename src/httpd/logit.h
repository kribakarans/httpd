#ifndef __HTTPD_LOGIT_H__
#define __HTTPD_LOGIT_H__

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define RESET_COLOR "\033[0m"
#define RED_COLOR   "\033[1;31m"

#define DEBUGIT(x) do { if (debug == true) { x; } } while (0)

extern FILE *httpd_logfp;

#define httpd_logit_init(file) \
	do { \
		if (file == NULL) { \
			httpd_logfp = stdout; \
		} else { \
			httpd_logfp = fopen(file, "w+"); \
			if (httpd_logfp == NULL) { \
				fprintf(stderr, "ERROR: failed to init logger: fopen() failed for %s (%s)\n", \
				        file, strerror(errno)); \
				abort(); \
			} \
		} \
	} while(0);

#define httpd_logit_close() \
	do { \
		if (httpd_logfp != NULL) \
			fclose(httpd_logfp); \
	} while(0);

#define logit_enter()  logit("Enter")
#define logit_finish() logit("Finish")

#define logit_retval() \
	do { \
		if (retval != HTTPD_RETSXS) \
			logit("retval: %d", retval); \
	} while(0);

#define logit(...) \
	do { \
		if (httpd_logfp == NULL) \
			httpd_logfp = stdout; \
		fprintf(httpd_logfp, "DEBUG %d %17s:%-4d %25s() :: ", getpid(), __FILE__, __LINE__, __func__); \
		fprintf(httpd_logfp, __VA_ARGS__); \
		fprintf(httpd_logfp, "\n"); \
		fflush(httpd_logfp); \
	} while(0);

/* Print logs to console and log file */
#define logit_error(...) \
	do { \
		if (httpd_logfp == NULL) { \
			httpd_logfp = stderr; \
		} else { \
			fprintf(stderr, "ERROR %d %17s:%-4d %25s() :: ",  getpid(), __FILE__, __LINE__, __func__); \
			fprintf(stderr, __VA_ARGS__); \
			fprintf(stderr, "\n"); \
		} \
		fprintf(httpd_logfp, "ERROR %d %17s:%-4d %25s() :: ", getpid(), __FILE__, __LINE__, __func__); \
		fprintf(httpd_logfp, __VA_ARGS__); \
		fprintf(httpd_logfp, "\n"); \
	} while(0);

#define httpd_printf(...) \
	do { \
		fprintf(stdout, __VA_ARGS__); \
		fprintf(stdout, "\n"); \
	} while(0);

#define httpd_print_error(...)  \
	do { \
		if (httpd_logfp != NULL) \
			logit_error(__VA_ARGS__); \
		fprintf(stderr, RED_COLOR); \
		fprintf(stderr, __VA_ARGS__); \
		fprintf(stderr, RESET_COLOR "\n"); \
	} while (0)

#endif
