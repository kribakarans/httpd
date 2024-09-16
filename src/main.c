#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "logit.h"
#include "config.h"
#include "httpd/httpd.h"

#if 0
void index_handler(void *data) {
	printf("Index Page: %s\n", (char *)data);
}

void about_handler(void *data) {
	printf("About Page: %s\n", (char *)data);
}

void help_handler(void *data) {
	printf("Help Page: %s\n", (char *)data);
}
#endif

void test_root_handler(const int sockfd, void *data)
{
	printf("socket=%d\n", sockfd);
	printf("Test Page: %s\n", (char *)data);
	httpd_serve_file(sockfd, "/test.html");

	return;
}

void test_async_get_handler(const int sockfd, void *data)
{
	char time_str[20] = {0};
	time_t now = time(NULL);
	struct tm *tm_info = localtime(&now);

	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

	printf("socket=%d\n", sockfd);
	printf("Data: %s\n", (char *)data);

	printf("Sending GET response...\n");
	sleep(3);
	httpd_send(sockfd, "[%s] Helloworld!", time_str);
	printf("Done.\n");

	return;
}

void test_async_post_handler(const int sockfd, void *data)
{
	printf("socket=%d\n", sockfd);
	printf("Data: %s\n", (char *)data);

	return;
}

int main()
{
	int retval = -1;
	int portno = 8888;

	/* Register client request handlers */
	//httpd_route_set_handler("/", index_handler, "Welcome to the home page!");
	//httpd_route_set_handler("/about", about_handler, "This is the about page.");
	//httpd_route_set_handler("/help", help_handler, "Help is available here.");
	httpd_route_set_handler("/test", test_root_handler, "Welcome to the test page!");
	httpd_route_set_handler("/testget",  test_async_get_handler,  "Test async get");
	httpd_route_set_handler("/testpost", test_async_post_handler, "Test async post");

	/* Run web-server */
	retval = httpd_run(portno, NULL);
	if (retval != HTTPD_RETSXS) {
		printf("Oops! failed to run server.\n");
	}

	printf("retval=%d\n", retval);

	return retval;
}

/* EOF */
