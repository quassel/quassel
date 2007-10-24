#include <openssl/blowfish.h>




int main(int,char**)
	{
	BF_KEY key;
	unsigned char * foo=(unsigned char *)"hello world";
	BF_set_key(&key, 1,foo);
	return 0;
	}



