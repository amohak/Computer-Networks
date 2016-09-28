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

#define MAX_PENDING 100
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

	sigaction(SIGCHLD, &sigchld_action, NULL);		//handles death of child so that childCount is decremented after every child completes

	while(1)
	{

		/* For the first part, the proxy acts as the server accepting requests from the client */

		int new_s;
		len = 10;
		if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0) {
			continue;
		}


		childCount++ ;
		while(childCount > MAX_CHILD)				//If number of processes is greater than 20, the process waits
			wait();

		// cout << "No. of children now: " << childCount << "\n";

		/* As soon as a client is connected and number of threads alive is less than 20, create a thread to serve the client */
		if(fork() == 0)
		{
			char *requestmessage;			// contains the request message recieved by the proxy
			char temp_request[MSGRESLEN];
			char request_to_server[MSGRESLEN];		//contains the request message to be sent to the server by the proxy
			int l;
			char headers[MSGRESLEN];				// contains the headers sent by the client
			int temp_flag = 0;
			int requestmessage_length = 0;

			/* The loop makes sure split fetches work properly */
			while(1)
			{
				l = recv(new_s,temp_request,MSGRESLEN,0);
				if(l<=0)
				{
					close(new_s);
					exit(1);
				}

				requestmessage_length+=l;
				requestmessage = (char *)realloc(requestmessage,min(requestmessage_length, MAX_BUFFER_SIZE));
				if(!temp_flag)
				{
					strcpy(requestmessage,temp_request);
					temp_flag = 1;
				}

				else
				{
					strcat(requestmessage,temp_request);
				}

				if(requestmessage[requestmessage_length-4]=='\r' && requestmessage[requestmessage_length-3]=='\n' && requestmessage[requestmessage_length-2]=='\r' && requestmessage[requestmessage_length-1]=='\n') break;
			}

			ParsedRequest *req = ParsedRequest_create();

			/* The following "if" condition handles the bad parse error */

			if (ParsedRequest_parse(req, requestmessage, requestmessage_length) < 0) {
				char *buf = (char *)malloc(100);
				strcat(buf, "HTTP/1.0 500 Internal Error\r\n\r\n Internal Error Occured\n");
				send_new(new_s,buf,strlen(buf));
				close(new_s);
				exit(1);
			}

			if (ParsedHeader_set(req, "Connection", "close") < 0){			// Setting "Connection" header to CLOSE
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

			if (ParsedHeader_set(req, "Host",host_address) < 0){		// Setting "Host Address"
				perror("set header key not working\n");
				exit(1);
			}

			ParsedRequest_unparse_headers(req, headers, sizeof(headers));

			prepare_request(req->method,req->path,req->version,headers,request_to_server);	// concatenated request line and the headers sent by the client

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

			send_new(client_socket, request_to_server, strlen(request_to_server));	// sending the request to the server

			char buf[MAX_BUFFER_SIZE];
			memset(buf,'\0',sizeof(buf));

			int recv_len = 0;

			/* Whatever is received from the server is simply sent to the client */

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

			// Closing the connection
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
