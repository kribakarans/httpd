#include "httpd.h"
#include "logit.h"

FILE *logfp = NULL;

static void httpd_exit(void)
{
	logit("%s", "Bye");

	logit_close();

	return;
}

int httpd_init(void)
{
    int retval = -1;

    do {
        logit_init();

        logit("%s", "Hello");

        atexit(&httpd_exit);

        signal(SIGPIPE, SIG_IGN);

        retval = RETSXS;
    } while(0);

    return retval;
}

/* EOF */
