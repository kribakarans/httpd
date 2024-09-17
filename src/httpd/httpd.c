#include <stdarg.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include "httpd.h"
#include "logit.h"
#include "utils.h"
#include "threads.h"

//#define ENABLE_THREAD_POOL

#define MAX_ROUTES 100
#define MAX_CLIENTS 10
#define BUFFER_SIZE 4096
#define PUBLIC_FOLDER "public"

#define timer_init() \
	struct timeval start, end; \
	gettimeofday(&start, NULL); /* Get start time */

#define timer_stop() \
	gettimeofday(&end, NULL); /* Get end time */ \
	/* Calculate time difference in seconds and microseconds */ \
	double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

#define httpd_perror(fmt, ...) \
	logit_error(fmt ": %s", ##__VA_ARGS__, strerror(errno))

typedef struct thread_data_st {
	int index;
	int sockfd;
} thread_data_t;

size_t route_count = 0;
http_route_t route_table[MAX_ROUTES] = {0};

const char *http_header_200 = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
const char *http_header_404 = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h1>404 Page Not Found</h1>";
const char *http_header_200F = "HTTP/1.1 200 OK\r\nServer: klab-httpd\r\nContent-Type: %s\r\nContent-Length: %lu\r\n\r\n";

int httpd_route_set_handler(const char *route, http_handler_t handler, void *arg)
{
	logit("%s", route);

	if (route_count >= MAX_ROUTES) {
		httpd_print_error("Route limit reached (%d)", MAX_ROUTES);
		return -1;
	}

	if (!route || !handler) {
		httpd_print_error("Invalid route or handler");
		return -1;
	}

	// Add route to the static route table
	strncpy(route_table[route_count].route, route, sizeof(route_table[route_count].route) - 1);
	route_table[route_count].route[sizeof(route_table[route_count].route) - 1] = '\0';
	route_table[route_count].handler = handler;
	route_table[route_count].arg = arg;

	route_count++;

	return 0;
}

static ssize_t httpd_send_header(const int sockfd, const char *res_fmt, const char *file)
{
	bool debug = false;
	int retval = -1;
	ssize_t nbytes = -1;
	char *response = NULL;

	assert(res_fmt != NULL);

	logit("%s", file);

	retval = asprintf(&response, res_fmt, httpd_get_mimtype(file), httpd_get_file_content_length(file));
	assert(retval > 0);

	DEBUGIT(logit("%s", response));

	nbytes = write(sockfd, response, strlen(response));
	if (nbytes < 0) {
		httpd_perror("write() failed");
	}

	free(response);

	return nbytes;
}

static ssize_t httpd_send_status(const int sockfd, const char *response)
{
	bool debug = false;
	ssize_t nbytes = -1;

	assert(response != NULL);

	DEBUGIT(logit("%s", response));
	nbytes = write(sockfd, response, strlen(response));
	if (nbytes < 0) {
		httpd_perror("write() failed");
	}

	return nbytes;
}


/* Function to send formatted HTTP headers to the socket */
int httpd_send_header_v2(int sockfd, const char *fmt, ...)
{
	int fmtlen;
	va_list args;
	char buffer[BUFFER_SIZE] = {0};

	va_start(args, fmt);
	fmtlen = vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	if (fmtlen < 0) {
		return -1;
	}

	if (write(sockfd, buffer, fmtlen) == -1) {
		return -1;
	}

	return fmtlen;
}

ssize_t httpd_send_data(const int sockfd, const char *mimetype, const char *data, const size_t datalen)
{
	ssize_t nbytes = -1;

	logit("%s", data);

	/* Send the HTTP status first */
	nbytes = httpd_send_header_v2(sockfd, "HTTP/1.1 200 OK\r\nServer: klab-httpd\r\nContent-Type: %s\r\nContent-Length: %lu\r\n\r\n", mimetype, datalen);

	/* Write the formatted response to the socket */
	if ((nbytes = write(sockfd, data, datalen)) < 0) {
		httpd_perror("write() failed");
	}

	return nbytes;
}

ssize_t httpd_send(const int sockfd, const char *fmt, ...)
{
	va_list args;
	ssize_t nbytes = -1;
	char response[BUFFER_SIZE];

	assert(fmt != NULL);

	va_start(args, fmt);
	vsnprintf(response, sizeof(response), fmt, args);
	va_end(args);

	/* Send the HTTP status first */
	if ((nbytes = httpd_send_status(sockfd, http_header_200)) < 0) {
		httpd_perror("write() failed");
	}

	logit("%s", response);

	/* Write the formatted response to the socket */
	if ((nbytes = write(sockfd, response, strlen(response))) < 0) {
		httpd_perror("write() failed");
	}

	return nbytes;
}

void httpd_serve_file(const int sockfd, const char *route)
{
	FILE *fp = NULL;
	ssize_t nbytes = -1;
	char file[1024];
	char buffer[BUFFER_SIZE] = {0};

	do {
		snprintf(file, sizeof(file), "%s%s", PUBLIC_FOLDER, route);
		logit("file=%s", file);

		fp = fopen(file, "r");
		if (fp == NULL) {
			httpd_send_status(sockfd, http_header_404);
			break;
		}

		nbytes = httpd_send_header(sockfd, http_header_200F, file);
		if (nbytes < 0) {
			httpd_perror("write() failed");
			fclose(fp);
			break;
		}

		while (fgets(buffer, BUFFER_SIZE, fp) != NULL) {
			nbytes = write(sockfd, buffer, strlen(buffer));
			if (nbytes < 0) {
				httpd_perror("write() failed");
			}
		}

		fclose(fp);
	} while(0);

	return;
}

void handle_get_request(const int sockfd, const char *route)
{
	char file[1024] = {0};

	logit("socket=%d", sockfd);
	logit("route=%s", route);

	for (size_t i = 0; i < route_count; i++) {
		if (strcmp(route, route_table[i].route) == 0) {
			httpd_printf("Handling route: %s", route);
			route_table[i].handler(sockfd, route_table[i].arg);  // Call the handler with its argument
			logit_finish();
			return;
		}
	}

	/* Try to serve builtin index page if no root set */
	if (strcmp(route, "/") == 0) {
		httpd_serve_file(sockfd, "/index.html");
	}

	snprintf(file, sizeof(file), "%s%s", PUBLIC_FOLDER, route);
	if (access(file, F_OK) < 0) {
		httpd_send_status(sockfd, http_header_404);
	} else {
		httpd_serve_file(sockfd, route);
	}

	return;
}

void handle_post_request(const int sockfd, const char *route, const char *body)
{
	char file[1024] = {0};

	logit("socket=%d", sockfd);
	logit("route=%s", route);
	logit("body=%s", body);

	for (size_t i = 0; i < route_count; i++) {
		if (strcmp(route, route_table[i].route) == 0) {
			httpd_printf("Handling route: %s", route);
			route_table[i].handler(sockfd, route_table[i].arg);  // Call the handler with its argument
			logit_finish();
			return;
		}
	}

	/* Try to serve builtin index page if no root set */
	if (strcmp(route, "/") == 0) {
		httpd_serve_file(sockfd, "/index.html");
	}

	snprintf(file, sizeof(file), "%s%s", PUBLIC_FOLDER, route);
	if (access(file, F_OK) < 0) {
		httpd_send_status(sockfd, http_header_404);
	} else {
		httpd_serve_file(sockfd, route);
	}

	return;
}

static void httpd_handle_client(const int sockfd)
{
	bool debug = false;
	char *body = NULL;
	char *t_url = NULL;
	ssize_t nbytes = -1;
	char method[8], url[512];
	char header[BUFFER_SIZE] = {0};

	do {
		logit_enter();

		nbytes = read(sockfd, header, BUFFER_SIZE - 1);
		if (nbytes < 0) {
			httpd_perror("failed to read socket data");
			break;
		}

		header[nbytes] = '\0';
		logit("HTTP Request received:");
		DEBUGIT(logit("%s", header));

		sscanf(header, "%s %s", method, url);

		logit("%s %s", method, url);

		if (strcmp(method, "GET") == 0) {
			t_url = strtok(url, "?"); /* Trim the url from query (trim /entry.html from /entry.html?domain=google.com) */
			handle_get_request(sockfd, t_url);
		} else if (strcmp(method, "POST") == 0) {
			logit("%s", header);
			body = httpd_read_body(header);
			assert(body != NULL);
			handle_post_request(sockfd, url, body);
			free(body);
		}
	} while(0);

	close(sockfd);
	logit_finish();

	return;
}

static void httpd_handle_client_thread(void *data)
{
	thread_data_t *td_client = (thread_data_t*)data;

	assert(td_client != NULL);

	httpd_printf("[Task: %d] Task is running: socket=%d ...", td_client->index, td_client->sockfd);

	timer_init();

	httpd_handle_client(td_client->sockfd);

	timer_stop();

	httpd_printf("[Task: %d] Task completed: Time taken: %.2f seconds", td_client->index, elapsed_time);

	free(td_client);

	return;
}

static void httpd_handle_client_thread_stub(void *data)
{
	int load = rand() % 10 + 1;
	thread_data_t *td_client = (thread_data_t*)data;

	assert(td_client != NULL);

	httpd_printf("[Task: %d] Task is running: socket=%d. Work load %d seconds ...",
	              td_client->index, td_client->sockfd, load);

	timer_init();

	sleep(load);  /* Simulate workload */

	timer_stop();

	httpd_printf("[Task: %d] Task completed: Time taken: %.2f seconds ======", td_client->index, elapsed_time);

	free(td_client);

	return;
}

int httpd_run(const int portno, const char *logfile)
{
	int opt = 1;
	int retval = -1;
	int server_socket = -1;
	int client_socket = -1;
	char host[INET_ADDRSTRLEN] = {0};
	struct sockaddr_in server_addr = {0};
	struct sockaddr_in client_addr = {0};
	socklen_t client_addr_len = sizeof(client_addr);

#ifdef ENABLE_THREAD_POOL
	int index = 0;
	thread_data_t *thread_data = NULL;
#endif

	do {
		retval = httpd_init(logfile);
		if (retval != HTTPD_RETSXS) {
			httpd_print_error("HTTPd: Failed to init server.");
			retval = HTTPD_RETERR;
			break;
		}

		logit_enter();

		server_socket = socket(AF_INET, SOCK_STREAM, 0);
		if (server_socket < 0) {
			httpd_perror("HTTPd: Failed to create socket");
			exit(EXIT_FAILURE);
		}

		if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
			httpd_perror("HTTPd: Failed to set socketopt");
			retval = HTTPD_RETERR;
			break;
		}

		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = INADDR_ANY;
		server_addr.sin_port = htons(portno);

		if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
			httpd_print_error("HTTPd: Failed to bind address/port %d (%s)", portno, strerror(errno));
			retval = HTTPD_RETERR;
			break;
		}

		if (listen(server_socket, SOMAXCONN) < 0) {
			httpd_perror("HTTPd: Failed to listen port %d", portno);
			retval = HTTPD_RETERR;
			break;
		}

		/* Get human readable host ip address */
		inet_ntop(AF_INET, &server_addr.sin_addr, host, sizeof(host));
		logit("listening on port %s:%d", host, ntohs(server_addr.sin_port));
		httpd_printf("Server running on http://%s:%d\n", host, portno);

#ifdef ENABLE_THREAD_POOL
		httpd_threads = httpd_threadpool_init(/* NThreads */ 5, /* QueuseSize */ 10);
		if (httpd_threads == NULL) {
			logit_error("httpd_threadpool_init() failed");
			retval = HTTPD_RETERR;
			break;
		}
#endif

		while (true) {
			httpd_printf("\nWaiting fo new connection ...");
			client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
			if (client_socket < 0) {
				httpd_perror("HTTPd: Failed to accept client");
				continue;
			}

			httpd_printf("New client connected: %d", client_socket);

#ifdef ENABLE_THREAD_POOL
			thread_data = (thread_data_t*)malloc(sizeof(thread_data));
			assert(thread_data != NULL);

			thread_data->index  = index;
			thread_data->sockfd = client_socket;

			httpd_printf("[Task: %d] Adding task to the pool", index);

			if (httpd_threadpool_add_task(httpd_threads, httpd_handle_client_thread, (void *)thread_data) != 0) {
				logit_error("[Error] Failed to add task %d to the pool.", index);
			}

			index++;
#else
			httpd_handle_client(client_socket);
#endif
		}

#ifdef ENABLE_THREAD_POOL
		httpd_threadpool_clean(httpd_threads);
#endif
	} while(0);

	close(server_socket);
	logit_retval();
	logit_finish();

	return retval;
}

/* EOF */
