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

char filename[MAX_LINE];
char custom[] = "123";
char null[] = "NULL";

int main(int argc, char * argv[])
{
	struct hostent *hp;
	struct sockaddr_in sin;
	in_addr_t host;
	char buf[MAX_LINE];
	int s, SERVER_PORT, filesize, len1, len2, temp1, temp2;
	
	if (argc==3) {
		hp = gethostbyname(argv[1]);							// gets the ip address (not needed though for this assignment)
		SERVER_PORT = atoi(argv[2]);							// server port to listen
	}
	
	else {
		fprintf(stderr, "usage: outfile host_ip server_port\n");
		exit(1);
	}
	
	/* building address data structure */
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	memcpy((char *)&sin.sin_addr, hp->h_addr, hp->h_length);
	sin.sin_port = htons(SERVER_PORT);
	
	/* active open */
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket not created");
		exit(1);
	}
	
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("error in socket connection");
		close(s);
		exit(1);
	}
	
	FILE * stream = fdopen(s,"r+");

	while(1)
	{
		memset(buf,'\0',sizeof(buf));
		recv(s,buf,sizeof(buf),0);								// receiving welcome message
		fputs(buf,stdout);
		memset(filename,'\0',sizeof(filename));
		fgets(filename, sizeof(filename),stdin);				
		send(s,filename,sizeof(filename),0);					// requesting for a file to the server

		memset(buf,'\0',sizeof(buf));
		recv(s,buf,sizeof(buf),0);
		
		if(strcmp(buf,null)==0)
		{
			printf("No such file exists\n");
		}

		else
		{
			fflush(stream);
			fscanf(stream,"%d.4",&filesize);
			memset(buf,'\0',sizeof(buf));
			temp1 = 0;
			len1 = 0;
			int temp3;

			while(len1 != filesize)								// handling fread so that every byte is read from the stream
			{
				if(filesize - len1 >= sizeof(buf))
						temp3 = sizeof(buf);
					else
						temp3 = filesize -len1;

				temp1 = fread(buf, sizeof(char), temp3, stream);
				len2 = 0;

				while(len2 != temp1){							// handling fwrite so that every byte read is written
					temp2 = fwrite(buf, sizeof(char), temp1-len2, stdout);
					len2 += temp2;
					fflush(stdout);
				}

				len1 += temp1;
				
			}
		}

		send(s,custom,sizeof(custom),0);
	}

	return 0;
}