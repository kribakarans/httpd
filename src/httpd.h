#ifndef __HTTPD_H__
#define __HTTPD_H__

enum e_httpd_retval {
	RETERR = -1,
	RETSXS =  0,
};

int httpd_run(const int portno);

#endif
