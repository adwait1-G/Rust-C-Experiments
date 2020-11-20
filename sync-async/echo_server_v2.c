/*
 * echo_server_v2.c
 * 
 * An attempt to implement polling and use non-blocking calls 
 * to work our way through the blocking problem.
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
#include <stdbool.h>
#include <errno.h>

void poll_for_new_conn_requests (int server_fd, bool *fds_list)
{
    int                     ret = 0;
    int                     i = 0;
    int                     client_fd = 0;
    struct sockaddr_in      client_addr = {0};
    socklen_t               client_addr_len = 0;

    printf("poll_for_new_conn_requests invoked\n");

    // Simply call it 50 times.
    // Will help when there is a surge of new requests.
    for (i = 0; i < 50; i++)
    {
        // Poll for new requests by calling accept.
        ret = accept4(server_fd, (struct sockaddr *)&client_addr, &client_addr_len, SOCK_NONBLOCK);
        printf("accept4() returned %d\n", ret);
        if (ret > 0)
        {
            // Success case: We have a new socket!
            client_fd = ret;
            
            // Reaching here means accept ret has a new
            // descriptor. We handle only 1023 descriptors. Note it.
            if (client_fd >= 1024)
            {
                close(client_fd);
            }
            else
            {
                fds_list[client_fd] = true;
            }
        }
        else if (ret < 0)
        {   
            // Some error occured. What error?
            // errno will have the proper error code, check it.
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // Looks like there is no outstanding connection
                // request. So it is asking us to try later.
                printf("accept4() returned EAGAIN or EWOULDBLOCK. Try again later\n");
            }
            else
            {
                // If it is any other error, it means something else
                // went wrong. Kill!
                printf("accept4() failed\n");
                exit(-1);
            }
        }
    }
    printf("poll_for_new_conn_requests done\n");
}

void poll_for_client_data (bool *fds_list)
{
    int                     ret = 0;
    int                     i = 0, j = 0;
    int                     client_fd = 0;
    uint8_t                 request_buffer[10000] = {0};

    printf("poll_for_client_data invoked\n");

    // All numbers in fds_list may not be valid
    // descriptors. Check each one and if it is, call recv on it.
    for (i = 0; i < 1024; i++)
    {       
        // For every valid socket, call "recv" a 100 times.
        // Why?
        // If there is large amounts of data coming in from client,
        // calling a couple of times helps instead of calling
        // it just once.
        //
        // We have two choices:
        // 1. Calling recv 100 times on one socket and then
        //      moving forward to the next.
        // 2. Call recv once on one socket, then move to
        //      the next. Do this for 100 times.
        //
        // (2) seemed fairer and would result in a
        //      responsive server(considering all clients).
        // So going for it.
        for (j = 0; j < 100; j++)
        {
            // Check if it is a valid socket descriptor
            if (fds_list[i] == true)
            {
                // If it is, then call recv on it.
                ret = recv(i, request_buffer, sizeof(request_buffer), 0);
                printf("recv() on descriptor %d returned %d\n", i, ret);
                if (ret > 0)
                {
                    // recv returned success - it has received some data.
                    // We need to send back that data.
                    ret = send(i, request_buffer, sizeof(request_buffer), 0);
                    if (ret < 0)
                    {
                        // If send failed, let us close the connection,
                        // remove from our descriptor list.
                        printf("send() on descriptor %d failed\n", i);
                        close(i);
                        fds_list[i] = false;
                    }
                    else
                    {
                        printf("send() succeeded on %d\n", i);
                    }
                }
                else if (ret == 0)
                {
                    // If recv has returned 0,
                    // it means that the client has disconnected.
                    // Let us also cleanup.
                    close(i);
                    fds_list[i] = false;
                }
                else // ret < 0 
                {
                    // recv has errored out.
                    // Time to check errno.
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        printf("recv() returned EAGAIN or EWOULDBLOCK. Try again later\n");
                    }
                    else
                    {
                        // Something fatal. Kill the server.
                        printf("recv() failed\n");
                        exit(-1);
                    }
                }
            }
        }
    }
    printf("poll_for_client_data done\n");
}

int main (int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Usage: $ %s [host-ipv4-address] [port-number] [poll-time-interval]\n", argv[0]);
        return 0;
    }

    int                 server_fd = 0;
    int                 ret = 0;
    int                 i = 0;
    struct sockaddr_in  server_addr = {0};
    const char          *ip_addr = argv[1];
    uint16_t            port_no = atoi(argv[2]);
    bool                fds_list[1024] = {false};
    int                 poll_time_interval = atoi(argv[3]);

    // Lets create a socket.
    ret = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (ret < 0)
    {
        printf("socket() failed\n");
        return -1;
    }
    server_fd = ret;

    // Bind the socket to the passed (ip_address, port_no).
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    
    ret = bind(server_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        printf("bind() failed\n");
        return -1;
    }
    
    // Start listening
    ret = listen(server_fd, 50);
    if (ret < 0)
    {
        printf("listen() failed\n");
        return -1;
    }
    printf("Listening at (%s, %u)\n", ip_addr, port_no);

    // What do we do here?
    while (1)
    {   
        poll_for_new_conn_requests(server_fd, fds_list);
        poll_for_client_data(fds_list);
        printf("Polling done. Going back to sleep\n");
        sleep(poll_time_interval);
    }
}