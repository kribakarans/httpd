#include "httpd.h"
#include "logit.h"
#include "utils.h"

#define PORT 8888

int main()
{
    int retval = -1;

    do {
        retval = httpd_init();
        if (retval != RETSXS) {
            httpd_error("Failed to init server");
            retval = RETERR;
            break;
        }

        retval = httpd_run(PORT);
        if (retval != RETSXS) {
            httpd_error("Failed to run server");
            retval = RETERR;
            break;
        }
    } while(0);

    logit_retval();

    return retval;
}

/* EOF */
