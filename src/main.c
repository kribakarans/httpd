#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "httpd/httpd.h"

char *kpass_exec(const char *cmd);

void kpass_entry_handler(const int sockfd, void *data)
{
	printf("KPASS ENTRY HANDLER CALLED%d\n", *(int*)data);

	return;
}

void kpass_domain_handler(const int sockfd, void *data)
{
	char *output = kpass_exec("kpass -j -f 'NAME=google.com'");

	assert(output != NULL);

	httpd_send_data(sockfd, "text/json", output, strlen(output));

	free(output);

	return;
}

void kpass_list_handler(const int sockfd, void *data)
{
	char *output = kpass_exec("kpass -j -n -L");

	assert(output != NULL);

	httpd_send_data(sockfd, "text/json", output, strlen(output));

	free(output);

	return;
}

void index_handler(const int sockfd, void *data)
{
	printf("socket=%d\n", sockfd);
	printf("Test Page: %s\n", (char *)data);
	httpd_serve_file(sockfd, "/index.html");

	return;
}

int main()
{
	int retval = -1;
	int portno = 8888;

	/* Register client request handlers */
	httpd_route_set_handler("/", index_handler, "Home page");
	httpd_route_set_handler("/list", kpass_list_handler, "Kpass List");
	httpd_route_set_handler("/domain", kpass_domain_handler, "Kpass Domain");
	httpd_route_set_handler("/entry/:id", kpass_entry_handler, "Kpass Entry");

	/* Run web-server */
	retval = httpd_run(portno, NULL);
	if (retval != HTTPD_RETSXS) {
		printf("Oops! failed to run server.\n");
	}

	printf("retval=%d\n", retval);

	return retval;
}

char *kpass_exec(const char *cmd)
{
	FILE *fp = NULL;
	char *buffer = NULL;
	size_t temp_size = 0;
	char temp[1024] = {0};
	size_t total_size = 0;
	char *new_buffer = NULL;

	assert(cmd != NULL);

	fp = popen(cmd, "r");
	if (fp == NULL) {
		perror("popen failed");
		return NULL;
	}

	while (fgets(temp, sizeof(temp), fp) != NULL) {
		temp_size = strlen(temp);
		// Reallocate buffer to fit the new chunk
		new_buffer = realloc(buffer, total_size + temp_size + 1);
		if (new_buffer == NULL) {
			perror("realloc failed");
			free(buffer);
			pclose(fp);
			return NULL;
		}

		buffer = new_buffer;
		memcpy(buffer + total_size, temp, temp_size);
		total_size += temp_size;
		buffer[total_size] = '\0';
	}

	if (pclose(fp) < 0) {
		perror("pclose failed");
		return NULL;
	}

	return buffer;
}

/* EOF */
