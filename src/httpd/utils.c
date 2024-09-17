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

const char *httpd_get_mimtype(const char *file)
{
	const char *dot = NULL;
	const char *extn = NULL;
	const char *mimetype = NULL;

	do {
		assert(file != NULL);

		dot = strrchr(file, '.');

		if (!dot || dot == file) {
			// No valid extension found, use default type
			mimetype = "application/octet-stream";
			break;
		}

		extn = dot + 1; // Get the file extension (skip the dot)

		// Map file extension to content type
		if (strcmp(extn, "html") == 0 || strcmp(extn, "htm") == 0) {
			mimetype = "text/html";
		} else if (strcmp(extn, "css") == 0) {
			mimetype = "text/css";
		} else if (strcmp(extn, "js") == 0) {
			mimetype = "application/javascript";
		} else if (strcmp(extn, "jpg") == 0 || strcmp(extn, "jpeg") == 0) {
			mimetype = "image/jpeg";
		} else if (strcmp(extn, "png") == 0) {
			mimetype = "image/png";
		} else if (strcmp(extn, "gif") == 0) {
			mimetype = "image/gif";
		} else if (strcmp(extn, "json") == 0) {
			mimetype = "application/json";
		} else if (strcmp(extn, "xml") == 0) {
			mimetype = "application/xml";
		} else if (strcmp(extn, "txt") == 0) {
			mimetype = "text/plain";
		} else {
			mimetype = "application/octet-stream";  // Default binary type
		}
	} while(0);

	return mimetype;
}

long httpd_get_file_content_length(const char *file)
{
	struct stat st;

	// Get file statistics using stat()
	if (stat(file, &st) == 0) {
		// Return the file size in bytes
		return st.st_size;
	} else {
		// Return -1 if the file cannot be accessed
		perror("stat");
		return -1;
	}
}

/* Function to extract Content-Length from the HTTP headers */
int httpd_read_body_content_length(const char *header)
{
	const char *s_len = NULL;

	assert(header != NULL);

	s_len = strstr(header, "Content-Length:");

	if (s_len) {
		return atoi(s_len + 15);  /* Skip "Content-Length: " */
	}

	return 0;  /* No Content-Length found */
}

/* Function to read and return the body from the HTTP request */
char *httpd_read_body(const char *header)
{
	char *body = NULL;
	int content_length = 0;
	const char *body_start = NULL;
	const char *end_of_header = NULL;

	do {
		assert(header);

		/* Find the end of headers marked by \r\n\r\n */
		end_of_header = strstr(header, "\r\n\r\n");
		if (!end_of_header) {
			printf("Invalid HTTP request: No end of headers found.\n");
			break;
		}

		/* Move the pointer past the \r\n\r\n to the beginning of the body */
		body_start = end_of_header + 4;

		/* Get Content-Length to validate if we have the complete body */
		content_length = httpd_read_body_content_length(header);
		if (content_length < 0) {
			printf("Content-Length not found in headers.\n");
			break;
		}

		/* Allocate memory for the body and copy it from the header */
		body = (char *)malloc(content_length + 1);
		if (body == NULL) {
			printf("Memory allocation failed.\n");
			break;
		}

		/* Copy the body from the header (note: content_length bytes only) */
		strncpy(body, body_start, content_length);
		body[content_length] = '\0';
	} while(0);

	return body;
}

/* EOF */
