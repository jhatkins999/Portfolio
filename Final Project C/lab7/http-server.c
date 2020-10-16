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
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		die("signal() failed");

	if (argc != 5)
	{
		fprintf(stderr, "usage: <server_port> <web_root> <mdb-lookup-host> <mdb-lookup-port>\n");
		exit(1);
	}
//Connect to MDB Lookup Server
	//Create new socket connection
	unsigned short mdbport = atoi(argv[4]);
	struct hostent * he;
	if ((he = gethostbyname(argv[3])) == NULL)
		die("gethostbyname failed");
	//char *serverIP = inet_ntoa(*(struct in_addr *) he->h_addr);
	int mdbsock;
	if ((mdbsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		die("mdbsock failed");
	struct sockaddr_in mdbaddr;
	memset(&mdbaddr, 0, sizeof(mdbaddr));
	mdbaddr.sin_family = AF_INET;
	mdbaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	mdbaddr.sin_port = htons(mdbport);
	
	if (connect(mdbsock, (struct sockaddr *) &mdbaddr, sizeof(mdbaddr)) < 0)
		die("connection failed");

	FILE *mdbread = fdopen(mdbsock, "rb");


//Create the Server socket connection
	unsigned short serverport = atoi(argv[1]);
	// create a listneing server socket
	int servsock;
	if ((servsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		die("socket failed");
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr)); //Should I use memset What's it doing?
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any network interface
	servaddr.sin_port = htons(serverport);

	//Bind to local address
	if (bind(servsock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
		die("bind failed");
	if (listen(servsock, 50) < 0)
		die("listen failed");
	int clntsock;
	socklen_t clntlen;
	struct sockaddr_in clntaddr;
	memset(&clntaddr, 0, sizeof(clntaddr));
	
	//Create Error messages
	char E501[4096] = "HTTP/1.0 510 Not Implemented\r\n\r\n";
	strcat(E501, "<html><body><h1>501 Not Implemented</h1></body></html>\r\n");
	char E404[4096] = "HTTP/1.0 404 Not Found\r\n\r\n";
	strcat(E404, "<html><body><h1>404 Unable to open file</h1></body></html>\r\n");
	char E400[4096] = "HTTP/1.0 400 Bad Request\r\n\r\n";
	strcat(E400, "<html><body><h1>400 Bad Request</h1></body></html>\r\n");
	char E301[4096] = "HTTP/1.0 301 Moved Permanently\r\n";
	strcat(E301, "<html><head><title>301 Moved Permenately</title></head><body><h1> Moved Permenently</h1><p>The document has moved<a href=http://clac.cs.columbia.edu/~jha2137/cs3157/tng/>here</a></p>");

//Begin to accept connections. It will only accept one connection at a time. MAX 50.	
	while(1) 
	{
		//Recieve the connection
		clntlen = sizeof(clntaddr);
		if ((clntsock = accept(servsock, 
					(struct sockaddr *) &clntaddr,&clntlen)) < 0)
		{
			fprintf(stderr, "accept failed... trying again");
			continue;
		}
		/*
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(serverport);
		*/

		//Recieve the http request from the connected socket
		char buff[4096];
		read(clntsock, buff, 4096);
		char *token_separators = "\t \r\n";
		char *method = strtok(buff, token_separators);
		char *requestURI = strtok(NULL, token_separators);
		char *httpVersion = strtok(NULL, token_separators);
		char filename[256];

		//Get logging info
		char *IP;
	        IP = inet_ntoa(((struct sockaddr_in)  clntaddr).sin_addr);
		char log[256];	
		snprintf(log, 256, "%s '%s %s %s' ", IP, method, requestURI, httpVersion);
		
		//Error Checking
		//Get stats of request
		struct stat meta;
		char statlink[256];
		strcpy(statlink, argv[2]);
		strcat(statlink, requestURI);
		stat(statlink, &meta);
		//404 Error
		if (strstr("..", requestURI))
		{
			strcat(log, "404 ERROR");
			printf("%s\n", log);
			write(clntsock, E404, strlen(E404));
			close(clntsock);
			continue;
		}
		else if (strcmp(requestURI, "/cs3157/tng/") == 0)
		{
			strcat(log, "200 OK");
			printf("%s\n", log);
			strcpy(filename, argv[2]);
			strcat(filename, requestURI);
			strcat(filename, "index.html");
		}
		else if (meta.st_mode == S_IFDIR)
		{
			if (requestURI[strlen(requestURI) - 1] == 47)
			{
				strcat(log, "200 OK");
				printf("%s\n", log);
				strcpy(filename, argv[2]);
				strcat(filename, requestURI);
				strcat(filename, "/index.html");
			}
			//301 Error
			else
			{
				strcat(log, "301 ERROR");
				printf("%s\n", log);
				write(clntsock, E301, strlen(E301));
				close(clntsock);
				continue;
			}
		}
		//501 Error Wrong HTTP version -- not 1.0 or 1.1 
		else if(strstr(httpVersion, "HTTP/1.0") != 0 ||strstr(httpVersion, "HTTP/1.1") == 0)
		{
			strcat(log, "501 ERROR");
			printf("%s\n", log);
			write(clntsock, E501, strlen(E501));
			close(clntsock);
			continue;
		}
		//501 Error not get method
		else if(strcmp(method, "GET") != 0)
		{
			strcat(log, "501 ERROR");
			printf("%s\n", log);
			write(clntsock,E501, strlen(E501));
			close(clntsock);
			continue;
		}
		else if (requestURI[0] == 47 &&  strlen(requestURI) == 1)
		{
			strcat(log, "200 OK");
			printf("%s\n", log);
			strcpy(filename, argv[2]);
			strcat(filename, "/index.html");
		}
		//Check if the response contains mdb-lookup -- if it does loop here
		else if (strstr(requestURI, "mdb-lookup"))
		{
			char mdb_response[256];
			strcpy(mdb_response, "HTTP/1.0 200 0K \r\n\r\n");
			const char* form =
				"<h1>mdb-lookup</h1>\n"
				"<p>\n"
				"<form method =GET action=/mdb-lookup>\n"
				"lookup: <input type=text name=key>\n"
				"<input type=submit>\n"
				"</form>\n"
				"<p>\n";
			if (strstr(requestURI, "key"))
			{
				char *key = strstr(requestURI,"=");
				strcat(key, "\n");
				key++;
				char logkey[256];
				strncpy(logkey, key, strlen(key) - 1);
				send(mdbsock, key, strlen(key), 0);
				char line[256];
				write(clntsock, mdb_response, strlen(mdb_response));
			        write(clntsock, form, strlen(form));
				char *table =
					"<p>"
					"<p><table border>";
				write(clntsock, table, strlen(table));
				char *color;
				int c = 0;
				strcat(log, "200 OK");
				printf("looking up [%s]: %s\n", logkey, log);
				while(strcmp(fgets(line, 256, mdbread), "\n"))
				{
					if(c%2)
					{
						color = "yellow";
					}
					else{color = "white";}
					c++;
					char *tok;
					char *tok2;
					tok  = strtok(line, ":");
					tok2  = strtok(NULL, ":");
					char row[256];
					snprintf(row, 256, "<tr><td bgcolor=%s>%s: %s</tr></td>", color, tok, tok2);
					write(clntsock, row, strlen(row));

					
				}
				write(clntsock, "</table>\r\n", 8);
				close(clntsock);
				continue;
				
			}
			else
			{
				printf("%s\n", log);
				write(clntsock, mdb_response, strlen(mdb_response));
				write(clntsock, form, strlen(form));
				close(clntsock);
				continue;
			}

		}
		//Last Case -- Not mdb and not an error - just give them the file
		else
		{
			strcat(log, "200 OK");
			printf("%s\n", log);
			strcpy(filename, "");
			strcpy(filename, argv[2]);
			strcat(filename, requestURI);

		}	
		
		FILE *index;
	        if((index = fopen(filename, "rb")) == NULL)
		{
			write(clntsock, E404, strlen(E404));
			close(clntsock);
			continue;
		}
		struct stat stats;
		stat(filename, &stats);
		char response[4096];
		strcpy(response,"HTTP/1.0 200 OK \r\n");
		strcat(response, "\r\n");
		write(clntsock, response, strlen(response));
		int remaining = (int) stats.st_size;	
		char chunk[4096];
		int limit;

		 while(remaining > 0)
		 {	
			limit = remaining > sizeof(chunk) ? sizeof(chunk) : remaining;
			fread(chunk, limit, 1, index);
			if(write(clntsock, chunk, limit) != limit)
		 	{
				fprintf(stderr, "write failed to send all bytes\n");
				break;
			}	
		 	remaining -= limit;
		 }
	 
	close(clntsock);
	fclose(index);

	}

	return 0;

}
