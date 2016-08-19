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

string MediaType(char * a);

void initialise_ReasonPhrase();

void initialise_Message();

void status_line(char *version, int status_code, char * buf);

void prepare_response(char *connection_status,int content_length, string content_type, char *response_header);

void send_new(int sock,char *response_header,int len);

void sendfile_new(int sock,int f,int filesize);

char * LowerCase(char * a);

char * extract_type(char *filename);

void list_dir(DIR *dp, char *directory_list, char *dir_path);

void list_dir_file(DIR *dp, FILE *fp, char *dir_path);