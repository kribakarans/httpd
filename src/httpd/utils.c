#include "httpd.h"
#include "logit.h"

#include <signal.h>

FILE *httpd_logfp = NULL;

static void httpd_exit(void)
{
	logit("%s", "Bye");

	httpd_logit_close();

	return;
}

int httpd_init(const char *logfile)
{
	int retval = -1;

	do {
		httpd_logit_init(logfile);

		logit("%s", "Hello");

		atexit(&httpd_exit);

		signal(SIGPIPE, SIG_IGN);

		retval = HTTPD_RETSXS;
	} while(0);

	return retval;
}

/* EOF */
