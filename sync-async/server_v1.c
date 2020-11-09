/*
 * server_v1.c
 * 
 * Simple, serve one connection at a time server.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

void serve_connection (int client_fd)
{   
    uint8_t         request_buffer[10000] = {0};
    uint8_t         response_buffer[10000] = {0};
    int             ret = 0;

    // Only one request response!
    ret = recv(client_fd, request_buffer, sizeof(request_buffer), 0);
    if (ret < 0)
    {
        printf("recv() failed for fd = %d\n", ret);
        return;
    }

    // Print the request (symbolic of processing the request)
    printf("%d: %s\n", client_fd, request_buffer);

    // Send back response
    ret = send(client_fd, "Hello from server!", 19, 0);
    if (ret < 19)
    {
        printf("send() failed for fd = %d\n", ret);
        return;
    }

    // Done, go back.
    return;
}


int main (int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Usage: $ %s [host-ipv4-address] [port-number]\n", argv[0]);
        return 0;
    }

    int                 sock_fd = 0;
    int                 client_fd = 0;
    int                 ret = 0;
    int                 i = 0;
    struct sockaddr_in  server_addr = {0};
    struct sockaddr_in  client_addr = {0};
    socklen_t           client_addr_len = 0;
    const char          *ip_addr = argv[1];
    uint16_t            port_no = atoi(argv[2]);

    // Lets create a socket.
    ret = socket(AF_INET, SOCK_STREAM, 0);
    if (ret < 0)
    {
        printf("socket() failed\n");
        return -1;
    }
    sock_fd = ret;

    // Bind the socket to the passed (ip_address, port_no).
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    
    ret = bind(sock_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        printf("bind() failed\n");
        return -1;
    }
    
    // Start listening
    ret = listen(sock_fd, 50);
    if (ret < 0)
    {
        printf("listen() failed\n");
        return -1;
    }
    printf("Listening at (%s, %u)\n", ip_addr, port_no);

    // Do the thing
    while (1)
    {
        memset(&client_addr, '\0', sizeof(client_addr));
        
        // Wait till we get a connection request
        ret = accept(sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (ret < 0)
        {
            printf("accept() failed\n");
            return -1;
        }
        client_fd = ret;

        // Handle the request
        printf("Serving request for client no %d!\n", i);
        serve_connection(client_fd);

        // Once done, close it
        close(client_fd);
        i += 1;
    }
}