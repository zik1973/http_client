#pragma once

void error(const char *format, ...)
#ifdef __GNUC__
     __attribute__ ((__format__ (__printf__, 1, 2)));
#else
	 ;
#endif
