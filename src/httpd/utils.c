#include "httpd.h"
#include "logit.h"

#include <signal.h>

FILE *httpd_logfp = NULL;

static void httpd_exit(void)
{
	logit("%s", "Bye");

	httpd_logit_close();

	return;
}

int httpd_init(const char *logfile)
{
	int retval = -1;

	do {
		httpd_logit_init(logfile);

		logit("%s", "Hello");

		atexit(&httpd_exit);

		signal(SIGPIPE, SIG_IGN);

		retval = HTTPD_RETSXS;
	} while(0);

	return retval;
}

void printline(const char *buffer, const int line_number)
{
	int len = 0;
	char line[256];
	int current_line = 1;
	const char *start = buffer;
	const char *end   = buffer;

	// Find the start of the desired line
	while (*end != '\0') {
		if (current_line == line_number) {
			// Find the end of the current line (until we hit '\n' or '\0')
			while (*end != '\n' && *end != '\0') {
				end++;
			}

			// Temporarily null-terminate the string to print it with fputs
			len = end - start;
			if (len >= (int)sizeof(line)) {
				len = sizeof(line) - 1;  // Ensure no overflow
			}
			strncpy(line, start, len);
			line[len] = '\0';  // Null-terminate the extracted line

			// Use fputs to print the extracted line
			fputs(line, stdout);
			printf("\n");
			return;
		}

		// Move to the next line
		if (*end == '\n') {
			current_line++;
			start = end + 1;
		}
		end++;
	}

	// If we reach here, the line number is out of range
	//printf("Line %d not found!\n", line_number);

	return;
}

/* EOF */
