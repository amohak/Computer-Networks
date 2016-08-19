#include "book_keeping.h"

const char CRLF[] = "\r\n";

map<int, string> ReasonPhrase;

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

void send_new(int sock,char *response_header,int len)
{
	int rem_bytes = len;
	int sent_bytes = 0;
	int temp = 0;

	while((temp = send(sock,response_header+sent_bytes,rem_bytes,0)) > 0 && rem_bytes > 0) 
	{
		rem_bytes -= temp;
		sent_bytes += temp;
	}

	if(temp < 0)
	{
		perror("Error in sending response header");
		close(sock);
		exit(1);
	}
}

void sendfile_new(int sock,int f,int filesize)
{
	long int sent_bytes = 0;
	int rem_bytes = filesize;

	while(((sent_bytes = sendfile(sock,f,&sent_bytes,rem_bytes)) > 0) && (rem_bytes > 0)) rem_bytes -= sent_bytes;

	if(sent_bytes < 0)
	{
		perror("Error in sending the file");
		close(sock);
		exit(1);
	}
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