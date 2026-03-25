/* A simple echo server using TCP */
#include <stdio.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>


#define SERVER_TCP_PORT 3000	/* well-known port */
#define BUFLEN		100	/* buffer length set to 100 to meet requirements*/

int echod(int);
void reaper(int);

int main(int argc, char **argv)
{
	int 	sd, new_sd, client_len, port;
	struct	sockaddr_in server, client;

	switch(argc){
	case 1:
		port = SERVER_TCP_PORT;
		break;
	case 2:
		port = atoi(argv[1]);
		break;
	default:
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(1);
	}

	/* Create a stream socket	*/	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}

	/* Bind an address to the socket	*/
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}

	/* queue up to 5 connect requests  */
	listen(sd, 5);

	(void) signal(SIGCHLD, reaper);

	while(1) {
	  client_len = sizeof(client);
	  new_sd = accept(sd, (struct sockaddr *)&client, &client_len);
	  if(new_sd < 0){
	    fprintf(stderr, "Can't accept client \n");
	    exit(1);
	  }
	  switch (fork()){
	  case 0:		/* child */
		(void) close(sd);
		exit(echod(new_sd));
	  default:		/* parent */
		(void) close(new_sd);
		break;
	  case -1:
		fprintf(stderr, "fork: error\n");
	  }
	}
}

int echod(int sd)
{
	char buf[BUFLEN];
	int len = getReqFile(sd, buf, BUFLEN);
	if (len <= 0) {
		close(sd);
		return 1;  /* error or client closed */
	}

	/* Try to open the file */
	FILE *fp = fopen(buf, "r");
	if (fp == NULL) {
		/* Send error flag and message to client */
		char *error_flag = "E\n";
		char *error_msg = "File not found\n";
		write(sd, error_flag, strlen(error_flag));
		write(sd, error_msg, strlen(error_msg));
		close(sd);
		return 1;
	}

	/* Send success flag, then file contents to client */
	char *success_flag = "O\n";
	write(sd, success_flag, strlen(success_flag));
	size_t n;
	while ((n = fread(buf, 1, BUFLEN, fp)) > 0) {
		write(sd, buf, n);
	}

	fclose(fp);
	close(sd);
	return 0;
}

/*	requested file name retreival	*/
int getReqFile(int sd, char *fileName, size_t maxlen)
{
	ssize_t n;
    size_t total = 0;

    while (total < maxlen - 1) {
        n = read(sd, fileName + total, maxlen - 1 - total);
        if (n < 0) {
            return -1;   /* read error */
        }
        if (n == 0) {
            break;      /* client closed connection */
        }
        total += (size_t)n;
        if (fileName[total - 1] == '\n') {
            break;      /* optional: line-oriented termination */
        }
    }

    fileName[total] = '\0';
    return (int)total;
}

/*	reaper		*/
void	reaper(int sig)
{
	int	status;
	while(wait3(&status, WNOHANG, (struct rusage *)0) >= 0);
}
