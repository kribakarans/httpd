#ifndef __HTTPD_H__
#define __HTTPD_H__

enum e_httpd_retval {
	HTTPD_RETERR = -1,
	HTTPD_RETSXS =  0,
};

typedef void (*http_handler_t)(void *data);

typedef struct {
	char route[256];
	http_handler_t handler;
	void *arg;
} http_route_t;

extern http_route_t route_table[];

int httpd_init(const char *logfile);
int httpd_run(const int portno, const char *logfile);

void handle_request(const char *route);
int httpd_route_set_handler(const char *route, http_handler_t handler, void *arg);

#endif
