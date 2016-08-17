#include <bits/stdc++.h>
#include "proxy_parse.h"
using namespace std;

int main()
{
	const char *c = "GET /~psraj/ HTTP/1.1\r\nConnection: keep-alive\r\nHost: www.iitk.ac.in\r\nAccept-Encoding: gzip, deflate\r\nAccept: */*\r\nUser-Agent: HTTPie/0.9.2\r\n\r\n";
	int len = strlen(c); 
	//Create a ParsedRequest to use. This ParsedRequest is dynamically allocated.
	ParsedRequest *req = ParsedRequest_create();
	if (ParsedRequest_parse(req, c, len) < 0) {
	   printf("parse failed\n");
	   return -1;
	}

	printf("Method : %s\n", req->method);
	printf("File : %s\n", req->path);
	printf("Version : %s\n",req->version);
	// cout << req->headersused << "\n";
	// cout << req->headerslen << "\n";

	string requestedfile;

	ParsedHeader header;
	for(int i=0;i<req->headersused;i++)
	{
		header = *(req->headers+i);
		printf("%s : ",header.key);

		if(strcmp(header.key,"Host")==0)
			requestedfile = header.value;
		printf("%s\n",header.value);
	}

	requestedfile.append(req->path);

	// printf("%s\n",requestedfile);
	cout << requestedfile << "\n";
   return 0;
}