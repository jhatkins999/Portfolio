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

#include "mylist.h"
#include "mdb.h"

#define KeyMax 5


static void die(const char *message)
{
	perror(message);
	exit(1);
}

int main(int argc, char **argv)
{
	// ignore SIGPIPE so that we don't terminate when we call
	// send() on a disconnected socket
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		die("signal() failed");
	if (argc != 3)
	{
		fprintf(stderr, "usage: %s <server-port> <filebase>\n", argv[0]);
		exit(1);
	}

	unsigned short  port = atoi(argv[1]);
	//create a listening/server socket
		
	int servsock;
	if ((servsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		die("socket failed");

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //any network interface
	servaddr.sin_port = htons(port);

	// Bind to the local address
	if (bind(servsock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
		die("bind failed");
	//start listening
	if (listen(servsock, 5) < 0)
		die("listen failed");

	int clntsock;
	socklen_t clntlen;
	struct sockaddr_in clntaddr;

		
	//Accept an incoming connection
	while(1)
	{		
		clntlen = sizeof(clntaddr);
		if ((clntsock = accept(servsock, 
				(struct sockaddr *) &clntaddr, &clntlen)) < 0) 
			die("accept failed");
		FILE *input = fdopen(clntsock, "r");
	//Start of MDB Lookup Code
		
		char *databasefile = argv[2];
		FILE *datafp = fopen(databasefile, "rb");
		if(datafp == NULL)
			die("filename");
		struct List list;
		initList(&list);
		
		int loaded = loadmdb(datafp, &list);
		if (loaded < 0)
			die("loadedmdb");
		fclose(datafp);

		//Lookup Loop
		
		char line[1000];
		char key[KeyMax + 1];
		while (fgets(line, sizeof(line), input) != NULL)
		{
			strncpy(key, line, sizeof(key) - 1);
			key[sizeof(key) - 1] = '\0';
			size_t last = strlen(key) - 1;
			if (key[last] == '\n')
				key[last] = '\0';

			struct Node *node = list.head;
			int recNo = 1;
	
			while(node)
			{
				struct MdbRec *rec = (struct MdbRec *)node->data;
				if(strstr(rec->name, key) || strstr(rec->msg, key))
				{
					//INSERT NEW SEND HERE
					size_t len;
					char result[100];
					len = snprintf(result, 100, 
							"%4d: {%s} said {%s}\n",
						       	recNo, rec->name, rec->msg);
					send(clntsock, result, len, 0);
				}
				node = node->next;
				recNo++;
			}
		}
		freemdb(&list);	
		close(clntsock);
	}
}
