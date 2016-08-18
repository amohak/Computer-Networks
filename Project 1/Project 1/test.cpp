#include <bits/stdc++.h>
#include <sys/types.h>
#include <dirent.h>
#include "proxy_parse.h"
#include <sys/stat.h>
using namespace std;

#define MSGRESLEN 1024

char CRLF[] = "\r\n";

map<int, char *> ReasonPhrase;

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

void status_line(char *version, int status_code, char* buf)
{
	char temp[5];
	strcpy(buf,version);
	strcat(buf," ");
	sprintf(temp,"%d",status_code);
	strcat(buf,temp);
	strcat(buf," ");
	strcat(buf,ReasonPhrase[status_code]);
	strcat(buf,CRLF);
	return;
}

void prepare_response(char *connection_status,int content_length, string content_type, char*response_header)
{
	char *content_length_temp = new char[256];
	sprintf(content_length_temp,"%d",content_length);

	strcat(response_header,"Connection : ");
	strcat(response_header,connection_status);
	strcat(response_header,CRLF);
	strcat(response_header,"Content-Length : ");
	strcat(response_header,content_length_temp);
	strcat(response_header,CRLF);
	strcat(response_header,"Content-type : ");
	strcat(response_header,content_type.c_str());
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

void list_dir(DIR *dp)
{
	struct dirent *ep;
	if (dp != NULL)
		{
		  while (ep = readdir(dp))
			puts (ep->d_name);
			(void) closedir (dp);
		}
	else
		perror ("Couldn't open the directory");
}

int main()
{
	initialise_ReasonPhrase();	
	char *response_header = (char *)malloc(MSGRESLEN);
	char *connection_status;
	int content_length;
	char *temp_type;
	struct stat size_info;
	int filesize;

	const char *c = "GET /Project_1_CS425.pdf/ HTTP/1.1\r\nConnection: keep-alive\r\nHost: www.iitk.ac.in\r\nAccept-Encoding: gzip, deflate\r\nAccept: */*\r\nUser-Agent: HTTPie/0.9.2\r\n\r\n";
	int len = strlen(c); 
	ParsedRequest *req = ParsedRequest_create();
	if (ParsedRequest_parse(req, c, len) < 0) {
		status_line("HTTP/1.1",500,response_header);
		printf("parse failed\n");
		return -1;
	}

	ParsedHeader header;
	for(int i=0;i<req->headersused;i++)
	{
		header = *(req->headers+i);
		// printf("%s\n",header.key);
		// printf("%s\n",header.value);
		if(strcmp(LowerCase(header.key),"connection")==0)
		{
			connection_status = (char *)malloc(strlen(header.value));
			strcpy(connection_status,header.value);
		}
	}

	if(req->path[strlen(req->path)-1]=='/')				// trim the terminal slash character
		req->path[strlen(req->path)-1] = '\0';
		
	FILE * f = fopen(req->path,"r");
	
	if(f==NULL)
		printf("No such file exists\n");
	else
	{
		printf("File found\n");

		stat(req->path,&size_info);
				content_length = size_info.st_size;

		temp_type = extract_type(req->path);
		string content_type = MediaType(temp_type);
		prepare_response(connection_status,content_length,content_type,response_header);
		
		cout << response_header;

	}


	// DIR * dir = opendir(req->path);
	// if(dir)
	// 	printf("Directory exists\n");
	// else if (ENOENT == errno)
	// 	printf("No such directory exists\n");
	// else
	// 	printf("Some other reason\n");

	// list_dir(dir);


   return 0;
}
