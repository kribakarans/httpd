#ifndef __HTTPD_LOGIT_H__
#define __HTTPD_LOGIT_H__

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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

#define logit_close() \
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
		if (httpd_logfp == NULL) httpd_logfp = stderr; \
		fprintf(httpd_logfp, "DEBUG %d %17s:%-4d %25s() :: ", \
		        getpid(), __FILE__, __LINE__, __func__); \
		fprintf(httpd_logfp, __VA_ARGS__); \
		fprintf(httpd_logfp, "\n"); \
		fflush(httpd_logfp); \
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
