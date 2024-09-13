#include "httpd.h"
#include "logit.h"

#include <arpa/inet.h>

int httpd_run(const int portno)
{
    int opt = 1;
    int retval = -1;
    int server_socket = -1;
    int client_socket = -1;
    struct sockaddr_in server_addr = {0};
    struct sockaddr_in client_addr = {0};
    socklen_t client_len = sizeof(client_addr);

    do {
        logit_enter();

        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            httpd_error("Failed to create socket: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            httpd_error("setsockopt() failed: %s", strerror(errno));
            retval = RETERR;
            break;
        }

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(portno);

        if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            httpd_error("bind() failed: %s", strerror(errno));
            retval = RETERR;
            break;
        }

        if (listen(server_socket, SOMAXCONN) < 0) {
            httpd_error("listen() failed: %s", strerror(errno));
            retval = RETERR;
            break;
        }

        httpd_printf("Server running on %d\n", portno);

        while (1) {
            client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
            if (client_socket < 0) {
                httpd_error("accept() failed: %s", strerror(errno));
                continue;
            }

            httpd_printf("New client connected: %d", client_socket);
        }
    } while(0);

    close(server_socket);
    logit_retval();
    logit_finish();

    return 0;
}

/* EOF */
