#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_LINE 256

int main(int argc, char * argv[])
{
	FILE *fp;
	struct hostent *hp;
	struct sockaddr_in sin;
	in_addr_t host;
	char buf[MAX_LINE];
	int s, len, SERVER_PORT;
	
	if (argc==3) {
		hp = gethostbyname(argv[1]);
		SERVER_PORT = atoi(argv[2]);
	}
	
	else {
		fprintf(stderr, "usage: outfile host port\n");
		exit(1);
	}
	
	/* build address data structure */
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	memcpy((char *)&sin.sin_addr, hp->h_addr, hp->h_length);
	sin.sin_port = htons(SERVER_PORT);
	
	/* active open */
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("simplex-talk: socket");
		exit(1);
	}
	
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("simplex-talk: connect");
		close(s);
		exit(1);
	}
	
	/* main loop: get and send lines of text */
	FILE * stream = fdopen(s,"r+");

	while (fread(buf, sizeof(char), sizeof(buf), stdin)) {
		printf("WTF\n");
		buf[MAX_LINE-1] = '\0';
		// printf("Reached here\n");
		len = strlen(buf);
		// printf("%s\n",buf);
		// printf("%d\n",len);
		send(s, buf, len, 0);
		printf("Sent\n");
	}

	return 0;
}