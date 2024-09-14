#ifndef __HTTPD_H__
#define __HTTPD_H__

enum e_httpd_retval {
	HTTPD_RETERR = -1,
	HTTPD_RETSXS =  0,
};

int httpd_init(const char *logfile);
int httpd_run(const int portno, const char *logfile);

#endif
