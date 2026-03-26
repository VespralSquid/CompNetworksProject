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
void get_filename(int client_socket, char *filename, size_t size)
{
    printf("Enter filename to download: ");
    fgets(filename, size, stdin);
    // Remove newline character if present
    filename[strcspn(filename, "\n")] = '\0';
}

void send_filename(int server_socket, const char *filename)
{
    write(server_socket, filename, strlen(filename));
}

void receive_file(int server_socket, const char *filename)
{
    FILE *file = fopen(filename, "wb");
    if (file == NULL)
    {
        perror("fopen");
        return;
    }

    char buffer[BUFFER_SIZE + 1];
    ssize_t bytes_received;
    int first_packet = 1;

    while ((bytes_received = read(server_socket, buffer, sizeof(buffer))) > 0)
    {
        if (first_pocket)
        {
            // check if the first byte is a file marker
            if (buffer[0] == ERROR_MARKER)
            {
                printf("Error from server: %s\n", buffer + 1);
                fclose(file);
                remove(filename); // Remove the file if it was created
                return;
            }
            first_packet = 0;
        }
        // Write the received data to the file, skipping the first byte (marker)
        fwrite(buffer + 1, 1, bytes_received - 1, file);
    }

    if (bytes_received < 0)
    {
        perror("read");
    }
    else
    {
        print("File '%s' downloaded successfully.\n", filename);
    }

    fclose(file);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Connect to server
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(server_socket);
        return EXIT_FAILURE;
    }

    printf("Connected to server.\n");

    // Get filename from user and send to server
    char filename[256];
    get_filename(filename, sizeof(filename));
    send_filename(server_socket, filename);

    // Receive and handle the server response
    receive_file(server_socket, filename);

    close(server_socket);
    return EXIT_SUCCESS;
}