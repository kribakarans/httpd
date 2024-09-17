#ifndef __HTTPD_UTILS_H__
#define __HTTPD_UTILS_H__

const char *get_line(const char *buffer, int line_number);
void printline(const char *buffer, const int line_number);
const char *httpd_get_mimtype(const char *file);
long httpd_get_file_content_length(const char *file);
int httpd_read_body_content_length(const char *header);
char *httpd_read_body(const char *header);

#endif
