#include <assert.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include "httpd.h"
#include "logit.h"
#include "threads.h"

#define timer_init() \
	struct timeval start, end; \
	gettimeofday(&start, NULL); /* Get start time */

#define timer_stop() \
	gettimeofday(&end, NULL); /* Get end time */ \
	/* Calculate time difference in seconds and microseconds */ \
	double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

typedef struct thread_data_st {
	int index;
	int sockfd;
} thread_data_t;

void httpd_handle_client_thread(void *data)
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
	int index = 0;
	int server_socket = -1;
	int client_socket = -1;
	char host[INET_ADDRSTRLEN] = {0};
	thread_data_t *thread_data = NULL;
	struct sockaddr_in server_addr = {0};
	struct sockaddr_in client_addr = {0};
	socklen_t client_addr_len = sizeof(client_addr);

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
			httpd_print_error("HTTPd: Failed to create socket: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
			httpd_print_error("HTTPd: Failed to set socketopt: %s", strerror(errno));
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
			httpd_print_error("HTTPd: Failed to listen port %d: %s", portno, strerror(errno));
			retval = HTTPD_RETERR;
			break;
		}

		/* Get human readable host ip address */
		inet_ntop(AF_INET, &server_addr.sin_addr, host, sizeof(host));
		logit("listening on port %s:%d", host, ntohs(server_addr.sin_port));
		httpd_printf("Server running on %d\n", portno);

		httpd_threads = httpd_threadpool_init(/* NThreads */ 5, /* QueuseSize */ 10);
		if (httpd_threads == NULL) {
			httpd_print_error("httpd_threadpool_init() failed");
			retval = HTTPD_RETERR;
			break;
		}

		while (true) {
#if 1
			client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
			if (client_socket < 0) {
				httpd_print_error("HTTPd: Failed to accept client: %s", strerror(errno));
				continue;
			}

			httpd_printf("New client connected: %d", client_socket);
#else
			if (index >= 10) {
				pause();
			}
#endif

			thread_data = (thread_data_t*)malloc(sizeof(thread_data));
			assert(thread_data != NULL);

			thread_data->index  = index;
			thread_data->sockfd = index;

			httpd_printf("[Task: %d] Adding task to the pool", index);

			if (httpd_threadpool_add_task(httpd_threads, httpd_handle_client_thread, (void *)thread_data) != 0) {
				logit("[Error] Failed to add task %d to the pool.", index);
			}

			index++;
		}

		httpd_threadpool_clean(httpd_threads);
	} while(0);

	close(server_socket);
	logit_retval();
	logit_finish();

	return retval;
}

/* EOF */
