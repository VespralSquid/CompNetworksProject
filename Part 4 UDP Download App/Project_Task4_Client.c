#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 100

struct pdu
{
    char type;
    char data[BUFFER_SIZE];
};

void download_file(int client_socket, struct sockaddr_in *server_addr)
{
    struct pdu spdu, rpdu;
    socklen_t server_len = sizeof(*server_addr);

    // Get filename from user
    printf("Enter filename to download: ");
    spdu.type = 'C';
    scanf("%s", spdu.data);

    // Send filename PDU to server
    sendto(client_socket, &spdu, strlen(spdu.data) + 2, 0,
           (struct sockaddr *)server_addr, server_len);

    // Open local file to save downloaded data
    FILE *file = fopen(spdu.data, "wb");
    if (file == NULL)
    {
        perror("fopen");
        return;
    }

    // Receive PDUs from server until FINAL or ERROR
    while (1)
    {
        ssize_t bytes_received = recvfrom(client_socket, &rpdu, sizeof(rpdu), 0,
                                          (struct sockaddr *)server_addr, &server_len);
        if (bytes_received < 0)
        {
            perror("recvfrom");
            break;
        }

        if (rpdu.type == 'E')
        {
            // Error PDU - display message and clean up empty file
            printf("Error from server: %s\n", rpdu.data);
            fclose(file);
            remove(spdu.data);
            return;
        }
        else if (rpdu.type == 'D')
        {
            // Data PDU - write to file
            fwrite(rpdu.data, 1, bytes_received - 1, file);
        }
        else if (rpdu.type == 'F')
        {
            // Final PDU - write last chunk and stop
            fwrite(rpdu.data, 1, bytes_received - 1, file);
            printf("File '%s' downloaded successfully.\n", spdu.data);
            break;
        }
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

    // Create UDP socket
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0)
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
        close(client_socket);
        return EXIT_FAILURE;
    }

    printf("UDP File Download Client\n");

    // Menu loop - allows multiple downloads
    while (1)
    {
        printf("\n1. Download a file\n");
        printf("2. Quit\n");
        printf("Enter choice: ");

        int choice;
        scanf("%d", &choice);

        if (choice == 1)
        {
            download_file(client_socket, &server_addr);
        }
        else if (choice == 2)
        {
            printf("Goodbye!\n");
            break;
        }
        else
        {
            printf("Invalid choice, please try again.\n");
        }
    }

    close(client_socket);
    return EXIT_SUCCESS;
}