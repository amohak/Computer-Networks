#include <stdio.h>
#include <string.h>

int main()
{
	char str[] = "1,22,333,4444,55555\n";     
	char *token;
	char *rest = str;
	char *saveptr;

	printf("OK%s\n",strtok_r(rest,"\r\n",&saveptr));

	while((token = strtok_r(NULL, ",", &saveptr)))
	{
	    printf("token:%s\n", token);
	}
}