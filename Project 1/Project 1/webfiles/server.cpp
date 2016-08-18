#include <bits/stdc++.h>
#include <sys/types.h>
#include <dirent.h>
#include "proxy_parse.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>

#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#include <fcntl.h>

using namespace std;

#define MAX_PENDING 5
#define MSGRESLEN 8092
#define MAX_BUFFER_SIZE 8092000

char CRLF[] = "\r\n";

map<int, string> ReasonPhrase;

string htmlheader = "<HTML><BODY>";
string htmlfooter = "</BODY><HTML>";

string MediaType(char * a)
{
	if(strcmp(a,"html")==0) return "text/html";
	else if(strcmp(a,"htm")==0) return "text/html";
	else if(strcmp(a,"txt")==0) return "text/plain";
	else if(strcmp(a,"jpeg")==0) return "image/jpeg";
	else if(strcmp(a,"jpg")==0) return "image/jpeg";
	else if(strcmp(a,"gif")==0) return "text/gif";
	else if(strcmp(a,"pdf")==0) return "Application/pdf";
	else return "Application/octet-stream";
}

void initialise_ReasonPhrase()
{
	ReasonPhrase[200] = "OK";
	ReasonPhrase[400] = "Bad Request";
	ReasonPhrase[404] = "Not Found";
	ReasonPhrase[500] = "Internal Server Error";
	ReasonPhrase[501] = "Not Implemented";
}

void status_line(char *version, int status_code, char * buf)
{
	char temp[5];
	strcpy(buf,version);
	strcat(buf," ");
	sprintf(temp,"%d",status_code);
	strcat(buf,temp);
	strcat(buf," ");
	strcat(buf,ReasonPhrase[status_code].c_str());
	strcat(buf,CRLF);
}

void prepare_response(char *connection_status,int content_length, string content_type, char *response_header)
{
	char *content_length_temp = new char[256];
	sprintf(content_length_temp,"%d",content_length);
	strcat(response_header,"Connection : ");
	strcat(response_header,connection_status);
	strcat(response_header,CRLF);
	strcat(response_header,"Content-Length : ");
	strcat(response_header,content_length_temp);
	strcat(response_header,CRLF);
	strcat(response_header,"Content-Type : ");
	strcat(response_header,content_type.c_str());
	strcat(response_header,CRLF);
	strcat(response_header,CRLF);
	
	delete[] content_length_temp;
}

char * LowerCase(char * a)
{
	char *temp = (char *)malloc(strlen(a));
	int i;
	for(i=0;i<strlen(a);i++)
		temp[i] = tolower(a[i]);
	temp[i] = '\0';
	return temp;
}

char * extract_type(char *filename)
{
	int i = strlen(filename) - 1;
	while(i>=0 && filename[i] != '.') i--;
	i++;
	return filename+i;
}

void list_dir(DIR *dp, char *directory_list, char *dir_path)
{
	struct dirent *ep;
	if (dp != NULL)
		{
			while (ep = readdir(dp))
			{
				strcat(directory_list,"<a href=\"/");
				strcat(directory_list,dir_path);
				strcat(directory_list,"/");
				strcat(directory_list,ep->d_name);
				strcat(directory_list,"\">");
				strcat(directory_list,ep->d_name);
				strcat(directory_list,"</a><br>");
			}
			(void) closedir (dp);
		}
	else
		perror ("Couldn't open the directory");
}

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
		
		if((pid = fork()) < 0) perror("Error in forking");

		while((pid = fork()) == 0)
		{
			cout << "Inside the child of Connection #" << connection_count << "\n";

			// while(1)
			// {
				char *connection_status;
				int content_length, retval, type;
				char *temp_type;
				struct stat size_info, type_info;
				char *response_header = (char *)malloc(8092);

				char *requestmessage = (char *)malloc(8092);

				if(recv(new_s,requestmessage,8092,0) <= 0){					// receiving filename and checking for live client
					exit(1);
				}

				int l = strlen(requestmessage);
				// requestmessage[l] = '\0'; 

				ParsedRequest *req = ParsedRequest_create();
				if ((retval = ParsedRequest_parse(req, requestmessage, l)) < 0) {
					if(retval == -2)
						status_line("HTTP/1.1",501,response_header);
					else
						status_line("HTTP/1.1",500,response_header);

					printf("parse failed\n");
					return -1;
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
					perror("File/Directory not found");
					status_line(req->version,404,response_header);
				}
				else
				{
					if(S_ISREG(type_info.st_mode))
					{
						printf("File exists\n");
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
						
						len = send(new_s,response_header,strlen(response_header),0);
						if(len < 0)
						{
							perror("Error in sending response header");
							close(new_s);
							exit(1);
						}

						long int buf_size = 8092;
						long int sent_bytes = 0;
						int rem_bytes = content_length;

						while(((sent_bytes = sendfile(new_s,f,&sent_bytes,size_info.st_size)) > 0) && (rem_bytes > 0)) rem_bytes -= sent_bytes;

						if(sent_bytes < 0)
						{
							perror("Error in sending the file");
							close(new_s);
							exit(1);
						}

						free(response_header);

						close(f);
					}

					if(S_ISDIR(type_info.st_mode))
					{
						status_line(req->version,200,response_header);
						char directory_list[8092];

						memset(directory_list,'\0',8092);
						strcat(directory_list,htmlheader.c_str());
						DIR * dir = opendir(req->path);
						list_dir(dir,directory_list,req->path);
						strcat(directory_list,htmlfooter.c_str());

						content_length = strlen(directory_list);
						string content_type = MediaType("html");
						prepare_response(connection_status,content_length,content_type,response_header);

						len = send(new_s,response_header,strlen(response_header),0);
						if(len < 0)
						{
							perror("Error in sending response header");
							close(new_s);
							exit(1);
						}

						send(new_s,directory_list,strlen(directory_list),0);

						// cout << directory_list << endl;
					}
				}
			// }

			// close(new_s);
			// exit(0);
		}

		// close(new_s);
	}
	
	return 0;
}

