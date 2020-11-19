/*
 * echo_server_v0.c
 * 
 * Simple echo server which can handle only one client
 * at a time. Makes full use of blocking calls.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdbool.h>

// serve_connection can have different return values.
// Based on it, we need to take action in the main
// function.
enum 
{
    SERVE_CONN_FAILED,
    SERVE_CONN_SUCCESS,
    SERVE_CONN_CLIENT_DISCONN,
};

int serve_connection (int client_fd)
{   
    uint8_t         request_buffer[10000] = {0};
    int             ret = 0;
    int             req_len = 0;

    while (1)
    {
        // Block here till you get some data.
        ret = recv(client_fd, request_buffer, sizeof(request_buffer), 0);
        printf("recv ret = %d\n", ret);
        if (ret < 0)
        {
            printf("recv() failed for fd = %d\n", ret);
            return SERVE_CONN_FAILED;
        }
        else if (ret == 0)
        {
            // This is the case when the other side of the
            // connection has disconnected.
            return SERVE_CONN_CLIENT_DISCONN;
        }

        req_len = ret;

        // You send back the same data
        ret = send(client_fd, request_buffer, req_len, 0);
        printf("send() for descriptor %d return %d\n", client_fd, ret);
        if (ret < req_len)
        {
            printf("send() failed for fd = %d\n", ret);
            return SERVE_CONN_FAILED;
        }

        // This keeps happening till the client disconnects.
        // Pathetic isn't it?
    }

    // Done, go back.
    // Formality. Dead code.
    return SERVE_CONN_SUCCESS;
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

        ret = accept(sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (ret < 0)
        {
            printf("accept4() failed\n");
            return -1;
        }
        client_fd = ret;
        printf("Serving client_fd = %d\n", client_fd);
        serve_connection (client_fd);
        close(client_fd);
    }
}