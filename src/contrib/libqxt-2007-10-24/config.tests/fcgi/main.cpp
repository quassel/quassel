#include "fcgio.h"
#include "fcgi_config.h"
int main (int,char**)
	{
	FCGX_Request request;
        FCGX_Init();
        FCGX_InitRequest(&request, 0, 0);
	fcgi_streambuf(request.out);
	return 0;
	}


