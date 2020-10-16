#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

static void die(const char *s)
{
	perror(s);
	exit(1);
}

int main(int argc, char **argv)
{
	if (argc != 4)
	{
		fprintf(stderr, "usage: <host-name>, <port-number>, <file-path>\n");
		exit(1);
	}
	struct hostent *he;
	char *serverName = argv[1];
	// get the server ip from the name
	
	if ((he = gethostbyname(serverName)) == NULL)
		die("get host name failed");
	char *serverIP = inet_ntoa(*(struct in_addr *) he->h_addr);
	unsigned short port = atoi(argv[2]);
	//Create a new file for the downloaded file to go
	char * filename = strrchr(argv[3], '/');
	filename++;
	FILE *fp;
	if((fp = fopen(filename, "wb")) == NULL)
		die("File failed to be created");
	//Create a socket connection to the host web page	
	int sock;
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		die("socket failed");
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(serverIP);
	servaddr.sin_port = htons(port);

	if ((connect(sock, (struct sockaddr *) &servaddr, sizeof(servaddr))) < 0)
		die("connect failed");
	//Create the HTTP Request
	char headers[4096] = "Host: ";
	strcat(headers, argv[1]);
	strcat(headers, ":");
	strcat(headers, argv[2]);
	strcat(headers, "\r\n"); //Is it nescessary to have the new line after each one
	char http_request[4096] = "GET ";
	strcat(http_request, argv[3]);
	strcat(http_request, " HTTP/1.0\r\n");
	strcat(http_request, headers);
	strcat(http_request, "\r\n");
	//Send the HTTP Request to the specified webpage
	if(send(sock, http_request, sizeof(http_request),0) !=sizeof(http_request))
			die("http_request failed");
	//Recieve the response from the webpage and write it to file
	FILE * input = fdopen(sock, "rb");
 	char line[1000];
	int count = 0;
	while((fgets(line, 1000, input)) != NULL)
	{
		if(count++ == 0)
		{
			if(strstr(line, "200") && strstr(line, "OK"))
			{
				printf("%s", line);
			}
			else
			{
				printf("%s", line);
				exit(1);
			}
		}
		//Stops skipping if we've reached the break after the headers
		if((strcmp(line, "\r\n") == 0))
			break;
		if(count > 100)
			die("Too many headers boss something went wrong");

			
	}
	//Use fread to read the rest of the file and input it into new file
	//Check if at the end of the file
	char buff[1];
	while(fread(buff, sizeof(buff), 1, input))
	{
		fwrite(buff, sizeof(buff), 1, fp);
	}
	
	return 0;
}

