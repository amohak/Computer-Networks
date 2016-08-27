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

#define MAX_PENDING 20
#define MSGRESLEN 8092
#define MAX_BUFFER_SIZE 1000000

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
	while(1)
	{

		/* For the first part, the proxy acts as the server accepting requests from the client */

		int new_s;
		len = 10;
		if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0) {
			perror("error in accepting the connection");
			exit(1);
		}
		
		
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
				// cout << buf << "\n";
				send_new(new_s,buf,strlen(buf));
				close(new_s);
				exit(1);
			}

			if(req->port == NULL)
			{	
				req->port = (char *)malloc(3);				// Is it really needed? Yes, Indeed!
				strcpy(req->port,"80");
			}

			if (ParsedHeader_set(req, "Connection", "close") < 0){
				perror("set header key not working\n");
				exit(1);
			}

			char host_address[1000];
			strcat(host_address,req->host);
			strcat(host_address,":");
			strcat(host_address,req->port);

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
				perror("simplex-talk: socket");
				exit(1);
			}
			
			if (connect(client_socket, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
				perror("simplex-talk: connect");
				close(client_socket);
				exit(1);
			}

			// cout << requestmessage << "\n";

			send_new(client_socket, requestmessage, strlen(requestmessage));
				
			char *buf = (char *)malloc(MAX_BUFFER_SIZE);

			int recv_len = 0, bytes_received = 0;
			while((recv_len = recv(client_socket, buf + bytes_received, MAX_BUFFER_SIZE - 1 - bytes_received, 0))>0)
			{
				bytes_received += recv_len;
				// cout << "Received Length : " << recv_len << "\nBytes Received : " << bytes_received << "\n" << "Buffer\n";
				// cout << "Buffer Length : " << strlen(buf);
			}

			// cout << "Receive Length : " << recv_len << "\n";
			if(recv_len < 0)
			{
				perror("Error in socket connection");
				exit(1);
			}

			// cout << buf << "\n\nLength of the buffer : " << strlen(buf);

			send_new(new_s,buf,bytes_received);

			close(client_socket);
			close(new_s);
			exit(1);
		}

		else
		{
			close(new_s);
		}
	}	

	return 0;
}
