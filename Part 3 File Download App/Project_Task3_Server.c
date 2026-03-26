// imports
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define BUFFER_SIZE 100
#define FILE_MARKER 'F'
#define ERROR_MARKER 'E'

// functions
void send_error(int client_socket, const char *error_message)
{
    char buffer[BUFFER_SIZE + 1];
    buffer[0] = ERROR_MARKER;
    strncpy(buffer + 1, error_message, BUFFER_SIZE - 1);
    write(client_socket, buffer, strlen(error_message) + 1);
}

void send_file(int client_socket, const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        send_error(client_socket, "Error: File not found or cannot be opened");
        return;
    }

    char buffer[BUFFER_SIZE + 1];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
    {
        buffer[0] = FILE_MARKER;
        write(client_socket, buffer, bytes_read + 1);
    }

    fclose(file);
}

void get_filename(int client_socket, char *filename, size_t size)
{
    ssize_t bytes_received = read(client_socket, filename, size - 1);
    if (bytes_received < 0)
    {
        perror("read");
        filename[0] = '\0'; // Set filename to empty string on error
    }
    else
    {
        filename[bytes_received] = '\0'; // Null-terminate the filename
    }
}

void handle_client(int client_socket)
{
    char filename[256];

    get_filename(client_socket, filename, sizeof(filename));

    if (strlen(filename) == 0)
    {
        send_error(client_socket, "Error: No fielname recieved.");
        close(client_socket);
        return;
    }

    printf("Client requested file: %s\n", filename);
    send_file(client_socket, filename);
    close(client_socket);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(server_socket);
        return EXIT_FAILURE;
    }

    if (listen(server_socket, 5) < 0)
    {
        perror("listen");
        close(server_socket);
        return EXIT_FAILURE;
    }

    printf("Server is listening on port %d...\n", atoi(argv[1]));

    while (1)
    {
        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket < 0)
        {
            perror("accept");
            continue;
        }

        // Fork child process to handle client
        switch (fork())
        {
        case 0: // client process
            close(server_socket);
            handle_client(client_socket);
            exit(0);
        default: // parent process
            close(client_socket);
            wait(NULL);
            break;
        case -1:
            fprint(stderr, "fork: error\n");
        }
    }

    close(server_socket);
    return EXIT_SUCCESS;
}