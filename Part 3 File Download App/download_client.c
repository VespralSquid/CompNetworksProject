/* A download application client using TCP, built from echo client */
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>



#define SERVER_TCP_PORT 3000	/* well-known port */
#define BUFLEN		100	/* buffer length set to 100 to meet requirements*/

int main(int argc, char **argv)
{
	int 	n, i, bytes_to_read;
	int 	sd, port;
	struct	hostent		*hp;
	struct	sockaddr_in server;
	char	*host, *bp, rbuf[BUFLEN], sbuf[BUFLEN], filename[BUFLEN];

	switch(argc){
	case 2:
		host = argv[1];
		port = SERVER_TCP_PORT;
		break;
	case 3:
		host = argv[1];
		port = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
		exit(1);
	}

	/* Create a stream socket	*/	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}

	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if (hp = gethostbyname(host)) 
	  bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
	else if ( inet_aton(host, (struct in_addr *) &server.sin_addr) ){
	  fprintf(stderr, "Can't get server's address\n");
	  exit(1);
	}

	/* Connecting to the server */
	if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
	  fprintf(stderr, "Can't connect \n");
	  exit(1);
	}

    /* Get filename from user */
	printf("Requested filename: ");
	if (fgets(filename, BUFLEN, stdin) == NULL) {
		fprintf(stderr, "Error reading filename\n");
		close(sd);
		exit(1);
	}

	/* Send filename to server */
    filename[strcspn(filename, "\n")] = '\0';
	write(sd, filename, strlen(filename));

	/* Read response flag */
	char flag;
	n = read(sd, &flag, 1);
	if (n <= 0) {
		fprintf(stderr, "Error reading response\n");
		close(sd);
		exit(1);
	}

	if (flag == 'O') {
		/* Success: create file with requested name and write contents */
		FILE *fp = fopen(filename, "w");
		if (fp == NULL) {
			fprintf(stderr, "Can't create local file\n");
			close(sd);
			exit(1);
		}
		while ((n = read(sd, rbuf, BUFLEN)) > 0) {
			fwrite(rbuf, 1, n, fp);
		}
		fclose(fp);
		printf("File downloaded successfully as '%s'\n", filename);
	} else if (flag == 'E') {
		/* Error: read and display error message */
		while ((n = read(sd, rbuf, BUFLEN)) > 0) {
			write(1, rbuf, n);
		}
	} else {
		fprintf(stderr, "Unknown response from server\n");
	}

	close(sd);
	return(0);
}
