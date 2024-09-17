#ifndef __HTTPD_H__
#define __HTTPD_H__

#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>

enum e_httpd_retval {
	HTTPD_RETERR = -1,
	HTTPD_RETSXS =  0,
};

typedef void (*http_handler_t)(const int sockfd, void *data);

typedef struct {
	char route[256];
	http_handler_t handler;
	int sockfd;
	void *arg;
} http_route_t;

extern http_route_t route_table[];

int httpd_init(const char *logfile);
int httpd_run(const int portno, const char *logfile);

void handle_request(const int sockfd, const char *route);
void httpd_serve_file(const int sockfd, const char *file);
ssize_t httpd_send(const int sockfd, const char *format, ...);
int httpd_route_set_handler(const char *route, http_handler_t handler, void *arg);
ssize_t httpd_send_data(const int sockfd, const char *mimetype, const char *data, const size_t datalen);

#endif
