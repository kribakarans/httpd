#include <stdio.h>

#include "httpd/httpd.h"

int main()
{
    int retval = -1;
    int portno = 8888;

    retval = httpd_run(portno, "trace.log");
    if (retval != HTTPD_RETSXS) {
        printf("Oops! failed to run server.");
    }

    printf("retval=%d", retval);

    return retval;
}

/* EOF */
