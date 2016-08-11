#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>

#define MAX_PENDING 5
#define MAX_LINE 256

char welcome_message[] = "Welcome client !!\nPlease enter the file name : \0";
char file_prompt[] = "\nPlease enter the file name : \0";
char error_message[] = "No such file exists on server!\n\0";
char filename[MAX_LINE];
char custom[]="123";
char null[] = "NULL\0";

int main(int argc, char * argv[])
{
	sigignore(SIGPIPE);
	struct sockaddr_in sin;
	struct stat size_info;
	int filesize;
	char buf[MAX_LINE];
	unsigned int len;
	int s, new_s, SERVER_PORT, temp1, len1, len2, temp2, clientstatus=0;
	
	if (argc==2) {
		SERVER_PORT = atoi(argv[1]);
	}
	else
	{
		fprintf(stderr, "usage: outfile server_port\n");
		exit(1);
	}
	
	/* building address data structure */
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(SERVER_PORT);
	
	/* setup passive open */
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Cannot create the socket");
		exit(1);
	}

	if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0)
	{
		perror("Unabe to bind");
		exit(1);
	}

	listen(s, MAX_PENDING);
	/* wait for connection, then receive and print text */

	while(1)
	{
		len=10;
		if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0) {
			perror("error in accepting the connection");
			exit(1);
		}
		
		FILE * stream = fdopen(new_s,"r+");
		clientstatus = 1;

		if(send(new_s,welcome_message,strlen(welcome_message)+1,0) < 0){		// sending welcome message
			clientstatus = 0;
			close(new_s);
			continue;
		}
	
		
		while(1)
		{
			memset(filename,'\0',sizeof(filename));

			if(recv(new_s,filename,sizeof(filename),0) <= 0){					// receiving filename and checking for live client
				clientstatus = 0;
				break;
			}

			int temp = strlen(filename);
			filename[temp-1]='\0';
			FILE * fp = fopen(filename,"r");

			if(fp==NULL)
			{
				send(new_s,null,strlen(null)+1,0);								// sending error message to client
			}
			
			else
			{
				send(new_s,custom,sizeof(custom),0);
				stat(filename,&size_info);
				filesize = size_info.st_size;


				fprintf(stream,"%d.4",filesize);								// sending the size of the file requested

				if(ferror(stream))
				{
					clientstatus = 0;
					break;
				}

				temp1 = 0;
				len1 = 0;
				int temp3 = 0;

				while(len1 != filesize)										// handling fread so that every byte is read from the file
				{
					if(filesize - len1 >= sizeof(buf))
						temp3=sizeof(buf);
					else
						temp3=filesize - len1;
					temp1 = fread(buf, sizeof(char), temp3, fp);
					
					if(ferror(fp))
					{
						clientstatus = 0;
						break;
					}

					len2 = 0;

					while(len2 != temp1){									// handling fwrite so that every byte read is written to the stream
						temp2 = fwrite(buf, sizeof(char), temp1 - len2, stream);
						
						if(ferror(stream))
							{
								clientstatus = 0;
								break;
							}
						fflush(stream);
						len2 += temp2;
					}

					len1 += temp1;
					

					if(!clientstatus) break;								// clientstatus - variable to tell the liveness of client
				}

				fclose(fp);
			}

			if(clientstatus)
			{
				memset(custom,'\0',sizeof(custom));
				if(recv(new_s,custom,sizeof(custom),0) <=0)
				{
					clientstatus = 0;
					break;
				}

				if(send(new_s,file_prompt,strlen(file_prompt)+1,0)<0)			// send prompt for another message
				{
					clientstatus = 0;
					break;
				}
			}

			else break;
		}

		close(new_s);
	}
	return 0;
}