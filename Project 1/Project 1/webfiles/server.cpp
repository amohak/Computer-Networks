#include <bits/stdc++.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#include <fcntl.h>

#include "proxy_parse.h"
#include "book_keeping.h"

using namespace std;

#define MAX_PENDING 5
#define MSGRESLEN 8092
#define MAX_BUFFER_SIZE 8092

char BadRequestMessage[] = "Sorry! The page you're looking for doesn't exist.\0";
char *ParsingFailed[2];

void initialise_Message()
{
	ParsingFailed[0] = "Sorry! We didn't get the request properly. You may try to reload the page.\0";
	ParsingFailed[1] = "Sorry! We do not support the requested facility.\0";
}

string htmlheader = "<HTML><BODY>";
string htmlfooter = "</BODY><HTML>";

int main(int argc, char * argv[])
{
	int s,pid,new_s,SERVER_PORT;

	if (argc==2)
		SERVER_PORT = atoi(argv[1]);
	else
	{
		fprintf(stderr, "usage: outfile server_port\n");
		exit(1);
	}

	initialise_ReasonPhrase();
	initialise_Message();

	struct sockaddr_in sin;
	unsigned int len;

	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(SERVER_PORT);

	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Cannot create the socket");
		exit(1);
	}

	if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0)
	{
		perror("Unable to bind");
		exit(1);
	}

	listen(s, MAX_PENDING);

	int connection_count = 0;

	while(1)
	{
		len = 10;
		if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0) {
			perror("error in accepting the connection");
			exit(1);
		}

		connection_count++;
		cout << "Connection #" << connection_count << endl;
		
		while((pid = fork()) == 0)
		{
			cout << "Inside the child of Connection #" << connection_count << "\n";

				char *connection_status;
				int content_length, retval, type;
				char *temp_type;
				struct stat size_info, type_info;
				char *response_header = (char *)malloc(MAX_BUFFER_SIZE);

				char *requestmessage = (char *)malloc(MAX_BUFFER_SIZE);

				if(recv(new_s,requestmessage,MAX_BUFFER_SIZE,0) <= 0){					// receiving filename and checking for live client
					exit(1);
				}

				int l = strlen(requestmessage);
				requestmessage[l] = '\0'; 

				ParsedRequest *req = ParsedRequest_create();
				if ((retval = ParsedRequest_parse(req, requestmessage, l)) < 0) {
					if(retval == -2)
					{
						status_line("HTTP/1.1",501,response_header);
					}

					else
					{
						status_line("HTTP/1.1",400,response_header);
					}

					retval = abs(retval) - 1;

					prepare_response("keep-alive",strlen(ParsingFailed[retval]),"txt",response_header);
					send_new(new_s,response_header,strlen(response_header));
					send_new(new_s,ParsingFailed[retval],strlen(ParsingFailed[retval]));
					close(new_s);
					exit(0);
				}

				ParsedHeader header;
				for(int i=0;i<req->headersused;i++)
				{
					header = *(req->headers+i);
					if(strcmp(LowerCase(header.key),"connection")==0)
					{
						connection_status = (char *)malloc(strlen(header.value));
						strcpy(connection_status,header.value);
					}
				}

				if(req->path[strlen(req->path)-1]=='/')				// trim the terminal slash character
					req->path[strlen(req->path)-1] = '\0';
				
				type = stat(req->path,&type_info);

				int f = open(req->path,O_RDONLY);

				if(type != 0)
				{
					// perror("File/Directory not found");
					status_line(req->version,404,response_header);
					prepare_response(connection_status,strlen(BadRequestMessage),"txt",response_header);
					send_new(new_s,response_header,strlen(response_header));
					send_new(new_s,BadRequestMessage,strlen(BadRequestMessage));
				}
				else
				{
					if(S_ISREG(type_info.st_mode))
					{
						// printf("File exists\n");
						status_line(req->version,200,response_header);

						if(fstat(f,&size_info) < 0)
						{
							perror("Error in accessing file stats");
							close(new_s);
							exit(1);
						}

						content_length = size_info.st_size;
						temp_type = extract_type(req->path);
						string content_type = MediaType(temp_type);

						prepare_response(connection_status,content_length,content_type,response_header);				
						
						send_new(new_s,response_header,strlen(response_header));
						sendfile_new(new_s,f,content_length);

						close(f);
					}

					if(S_ISDIR(type_info.st_mode))
					{
						status_line(req->version,200,response_header);

						char directory_list[8092];
						memset(directory_list,'\0',8092);
						DIR * dir = opendir(req->path);
						
						strcat(directory_list,htmlheader.c_str());
						list_dir(dir,directory_list,req->path);
						strcat(directory_list,htmlfooter.c_str());

						content_length = strlen(directory_list);
						string content_type = MediaType("html");
						prepare_response(connection_status,content_length,content_type,response_header);
						send_new(new_s,response_header,strlen(response_header));
						send_new(new_s,directory_list,strlen(directory_list));
					}
				}
		}
	}
	return 0;
}

