#include <assert.h>
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

size_t route_count = 0;
http_route_t route_table[MAX_ROUTES] = {0};

int httpd_route_set_handler(const char *route, http_handler_t handler, void *arg)
{
	logit("%s", route);

	if (route_count >= MAX_ROUTES) {
		fprintf(stderr, "Error: Route limit reached (%d)\n", MAX_ROUTES);
		return -1;
	}

	if (!route || !handler) {
		fprintf(stderr, "Error: Invalid route or handler\n");
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

void handle_request(const char *route)
{
	for (size_t i = 0; i < route_count; i++) {
		if (strcmp(route, route_table[i].route) == 0) {
			printf("Handling route: %s\n", route);
			route_table[i].handler(route_table[i].arg);  // Call the handler with its argument
			return;
		}
	}

	printf("404 Not Found: %s\n", route);
}

typedef struct thread_data_st {
	int index;
	int sockfd;
} thread_data_t;

const char *http_header_200 = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
const char *http_header_404 = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404: File not found";

static ssize_t httpd_send_status(const int sockfd, const char *response)
{
	ssize_t nbytes = -1;

	assert(response != NULL);

	nbytes = write(sockfd, response, strlen(response));
	if (nbytes < 0) {
		httpd_perror("write() failed");
	}

	return nbytes;
}

static void httpd_serve_file(const int sockfd, const char *file)
{
	FILE *fp = NULL;
	ssize_t nbytes = -1;
	char relpath[1024];
	char buffer[BUFFER_SIZE] = {0};

	do {
		logit_enter();

		snprintf(relpath, sizeof(relpath), "%s%s", PUBLIC_FOLDER, file);
		logit("file=%s", relpath);

		fp = fopen(relpath, "r");
		if (fp == NULL) {
			httpd_send_status(sockfd, http_header_404);
			break;
		}

		nbytes = httpd_send_status(sockfd, http_header_200);
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

	logit_finish();

	return;
}

static void httpd_handle_client(const int sockfd)
{
	bool debug = false;
	ssize_t nbytes = -1;
	char method[8], path[512];
	char buffer[BUFFER_SIZE] = {0};

	do {
		logit_enter();

		nbytes = read(sockfd, buffer, BUFFER_SIZE - 1);
		if (nbytes < 0) {
			httpd_perror("failed to read socket data");
			break;
		}

		buffer[nbytes] = '\0';
		logit("HTTP Request received:");
		//logit("%s", buffer);

		sscanf(buffer, "%s %s", method, path);

		/* By default use builtin index page    */
		/* Might override by client application */
		if (strcmp(path, "/") == 0) {
			httpd_serve_file(sockfd, "/index.html");
			break;
		}

		logit("%s %s", method, path);

		if (strcmp(method, "GET") == 0) {
			handle_request(path);
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
