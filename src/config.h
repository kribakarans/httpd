#ifndef __HTTPD_CONFIG_H__
#define __HTTPD_CONFIG_H__

#define PROGRAM_NAME    "HTTPd"
#define PROGRAM_VERSION "0.1a"

#define HTTPDROOT ".httpd"

#ifndef TERMUX
	#define PREFIX "/tmp/"
#else
	#define PREFIX "/data/data/com.termux/files/usr/tmp/"
#endif

#define LOGFILE PREFIX "httpd.log"

#endif
