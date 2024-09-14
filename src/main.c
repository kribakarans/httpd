#include <stdio.h>

#include "config.h"
#include "httpd/httpd.h"

int main()
{
    int retval = -1;
    int portno = 8888;

    retval = httpd_run(portno, LOGFILE);
    if (retval != HTTPD_RETSXS) {
        printf("Oops! failed to run server.\n");
    }

    printf("retval=%d\n", retval);

    return retval;
}

/* EOF */
