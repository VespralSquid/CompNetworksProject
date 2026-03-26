#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define BUFFER_SIZE 100

struct pdu
{
    char type;
    char data[BUFFER_SIZE];
};

// Get file size using stat
off_t get_file_size(const char *filename)
{
    struct stat st;
    if (stat(filename, &st) == 0)
    {
        return st.st_size;
    }
    return -1;
}

void send_file(int server_socket, const char *filename,
               struct sockaddr_in *client_addr, socklen_t client_len)
{
    struct pdu spdu;

    // Try to open the file
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        // Send error PDU
        spdu.type = 'E';
        strncpy(spdu.data, "Error: File not found or cannot be opened.", BUFFER_SIZE - 1);
        sendto(server_socket, &spdu, sizeof(spdu), 0,
               (struct sockaddr *)client_addr, client_len);
        return;
    }

    // Get file size to detect last chunk
    off_t file_size = get_file_size(filename);
    off_t bytes_sent_total = 0;
    size_t bytes_read;

    while ((bytes_read = fread(spdu.data, 1, BUFFER_SIZE, file)) > 0)
    {
        bytes_sent_total += bytes_read;

        // If this is the last chunk, send as FINAL PDU
        if (bytes_sent_total >= file_size)
        {
            spdu.type = 'F';
        }
        else
        {
            spdu.type = 'D';
        }

        sendto(server_socket, &spdu, bytes_read + 1, 0,
               (struct sockaddr *)client_addr, client_len);
    }

    fclose(file);
    printf("File '%s' sent successfully.\n", filename);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Create UDP socket
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
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

    printf("UDP Server listening on port %d...\n", atoi(argv[1]));

    // Server loop - handles one request at a time (iterative, not concurrent)
    while (1)
    {
        struct pdu rpdu;
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        // Wait for a filename PDU from a client
        ssize_t bytes_received = recvfrom(server_socket, &rpdu, sizeof(rpdu), 0,
                                          (struct sockaddr *)&client_addr, &client_len);
        if (bytes_received < 0)
        {
            perror("recvfrom");
            continue;
        }

        // Check that it is a filename PDU
        if (rpdu.type == 'C')
        {
            printf("Client requested file: %s\n", rpdu.data);
            send_file(server_socket, rpdu.data, &client_addr, client_len);
        }
        else
        {
            printf("Unknown PDU type received: %c\n", rpdu.type);
        }
    }

    close(server_socket);
    return EXIT_SUCCESS;
}