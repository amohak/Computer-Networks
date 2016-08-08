#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX_PENDING 5
#define MAX_LINE 256

int main(int argc, char * argv[])
{
	struct sockaddr_in sin;
	char buf[MAX_LINE];
	unsigned int len;
	int s, new_s, SERVER_PORT;
	
	if (argc==2) {
		SERVER_PORT = atoi(argv[1]);
	}
	else
	{
		fprintf(stderr, "usage: outfile port\n");
		exit(1);
	}
	
	/* build address data structure */
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(SERVER_PORT);
	
	/* setup passive open */
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("simplex-talk: socket");
		exit(1);
	}

	if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0)
	{
		perror("simplex-talk: bind");
		exit(1);
	}

	listen(s, MAX_PENDING);
	/* wait for connection, then receive and print text */
	
	int i=0;
	while(1) {
		if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0) {
			perror("simplex-talk: accept");
			exit(1);
		}
		
		while (len = recv(new_s, buf, sizeof(buf), 0))
		{
			// i = 0;
			// while(buf[i]!='\0')
			// 	printf("%c ",buf[i++]);
			
			fputs(buf, stdout);
			memset(buf,'\0',len);
		}
		close(new_s);
	}
	return 0;
}