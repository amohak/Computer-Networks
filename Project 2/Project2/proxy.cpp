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

int childCount = 0;
void fun(int x)
{
	childCount--;
}

#define MAX_PENDING 20
#define MSGRESLEN 8192
#define MAX_BUFFER_SIZE 8192
#define MAX_CHILD 20

int main(int argc, char * argv[]) 
{
	int s,SERVER_PORT;
	if (argc==2)
		SERVER_PORT = atoi(argv[1]);
	else
	{
		fprintf(stderr, "usage: outfile server_port\n");
		exit(1);
	}

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
	struct sigaction sigchld_action;
	sigchld_action.sa_handler = fun;
	sigchld_action.sa_flags = SA_NOCLDWAIT;

	sigaction(SIGCHLD, &sigchld_action, NULL);

	int stat;
	while(1)
	{

		/* For the first part, the proxy acts as the server accepting requests from the client */

		int new_s;
		pid_t pid;
		len = 10;
		if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0) {
			// perror("error in accepting the connection");
			continue;
		}
		
		
		childCount++ ;
		while(childCount > MAX_CHILD)
		{
			wait(&stat);
		}

		cout << "No. of children now: " << childCount << "\n";
		/* As soon as a client is connected, create a thread to serve the client */
		if(fork() == 0)
		{
			char *requestmessage = (char *)malloc(MSGRESLEN);			// contains request line, and optionally, connection status and host address
			char request_to_server[MSGRESLEN];
			int l;												
			char headers[MSGRESLEN];													// contains the headers sent by the client
			if(( l = recv(new_s,requestmessage,MSGRESLEN,0)) <= 0){
				close(new_s);
				exit(1);
			}

			ParsedRequest *req = ParsedRequest_create();

			if (ParsedRequest_parse(req, requestmessage, l) < 0) {
				char *buf = (char *)malloc(100);
				strcat(buf, "HTTP/1.0 500 Internal Error\r\n\r\n Internal Error Occured\n");
				send_new(new_s,buf,strlen(buf));
				close(new_s);
				exit(1);
			}

			if (ParsedHeader_set(req, "Connection", "close") < 0){
				perror("set header key not working\n");
				exit(1);
			}

			char host_address[1000];
			strcat(host_address,req->host);
			
			if(req->port != NULL)
			{
				strcat(host_address,":");
				strcat(host_address,req->port);
			}
			else
			{
				req->port = (char *)malloc(3);
				strcpy(req->port,"80");
			}

			if (ParsedHeader_set(req, "Host",host_address) < 0){
				perror("set header key not working\n");
				exit(1);
			}

			ParsedRequest_unparse_headers(req, headers, sizeof(headers));

			prepare_request(req->method,req->path,req->version,headers,request_to_server);
			
			// cout << request_to_server << "\n";			
			
			/* Now the proxy acts as the client and will be sending the request to the server */

			struct hostent *hp;
			struct sockaddr_in sin;

			hp = gethostbyname(req->host);
			memset((char *)&sin, 0, sizeof(sin));
			sin.sin_family = AF_INET;
			memcpy((char *)&sin.sin_addr, hp->h_addr, hp->h_length);
			sin.sin_port = htons(atoi(req->port));
			

			int client_socket;
			if ((client_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
				perror("error in creating socket");
				exit(1);
			}
			
			if (connect(client_socket, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
				perror("error in socket connection");
				close(client_socket);
				exit(1);
			}

			send_new(client_socket, request_to_server, strlen(request_to_server));
				
			char buf[MAX_BUFFER_SIZE];
			memset(buf,'\0',sizeof(buf));

			int recv_len = 0; 
			while((recv_len = recv(client_socket, buf, MAX_BUFFER_SIZE - 1, 0))>0)
			{
				send_new(new_s,buf,recv_len);
				memset(buf,'\0',sizeof(buf));
			}

			if(recv_len < 0)
			{
				perror("Error in receiving buffer");
				exit(1);
			}

			close(client_socket);
			close(new_s);
			exit(0);
		}

		else
		{
			close(new_s);
		}
	}	

	return 0;
}
