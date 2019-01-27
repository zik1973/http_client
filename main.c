#include "http.h"

#ifdef UNIT_TEST
int main()
{
	//test_url_parse();
	test_http();
	return 0;
}
#else
int main(int argc, char *argv[])
{
	return 0;
}
#endif
