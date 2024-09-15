#include <stdio.h>

#include "config.h"
#include "httpd/httpd.h"

void index_handler(void *data) {
    printf("Index Page: %s\n", (char *)data);
}

void about_handler(void *data) {
    printf("About Page: %s\n", (char *)data);
}

void help_handler(void *data) {
    printf("Help Page: %s\n", (char *)data);
}

int main()
{
    int retval = -1;
    int portno = 8888;

    /* Register client request handlers */
    httpd_route_set_handler("/", index_handler, "Welcome to the home page!");
    httpd_route_set_handler("/about", about_handler, "This is the about page.");
    httpd_route_set_handler("/help", help_handler, "Help is available here.");

    /* Run web-server */
    retval = httpd_run(portno, NULL);
    if (retval != HTTPD_RETSXS) {
        printf("Oops! failed to run server.\n");
    }

    printf("retval=%d\n", retval);

    return retval;
}

/* EOF */
