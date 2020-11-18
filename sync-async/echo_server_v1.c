/*
 * server_v1.c
 * 
 * Simple I/O multiplexing using select.
 * - Server simply receives data and sends some data to client.
 * - server_v5.c extends on this concept and an echo server is written.
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

	printf("Inside serve_connection for descriptor %d\n", client_fd);

    // Recv it
    // What if the client sends more than 10,000 bytes of data?
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

    // Send back response
    // Can this be blocking?
    ret = send(client_fd, request_buffer, req_len, 0);
    printf("send() for descriptor %d return %d\n", client_fd, ret);
    if (ret < req_len)
    {
        printf("send() failed for fd = %d\n", ret);
        return SERVE_CONN_FAILED;
    }

    // Done, go back.
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
    fd_set              read_set = {0};
    bool                fds_list[FD_SETSIZE] = {false};

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

    // Before entering, initialize everything we need to call select.
    // This set is passed to select, it gets altered when select succeeds.
    FD_ZERO(&read_set);

    // Do the thing
    while (1)
    {
        memset(&client_addr, '\0', sizeof(client_addr));

        // Copy.
        FD_SET(sock_fd, &read_set);

        printf("Waiting for select() to succeed\n");
        ret = select(FD_SETSIZE, &read_set, NULL, NULL, NULL);
        if (ret <= 0)
        {
            printf("select() failed\n");
            return -1;
        }
        printf("After select, %d descriptors are ready!\n", ret);
        // Suppose it succeeds, we don't know which sockets are ready.
        // All the "ready" sockets are set in those sets.
        // We know the socket value, we need to go over them and see
        // which one to process first.
        
        // If this comes true, that means we have an incoming connection
        // waiting to be accepted.
        if (FD_ISSET(sock_fd, &read_set))
        {   
            printf("Inside sock_fd if\n");
            ret = accept(sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);
            if (ret < 0)
            {
                printf("accept4() failed\n");
                return -1;
            }
            client_fd = ret;
            printf("client_fd = %d\n", client_fd);

			// select won't be able to handle any descriptor with value
			// >= FD_SETSIZE (which is 1024). Check it and kill it
			// if the descriptor is greater.
			if (client_fd >= FD_SETSIZE)
			{
				close (client_fd);
			}
			else
			{
            	// We want select to keep an eye on this new socket.
            	fds_list[client_fd] = true;
			}

            // When select returns, read_set would be modified
            // to keep just the "ready" descriptors. A couple of them
            // might be lost. Need to update read_set before
            // we pass it to select again.
            for (i = 0; i < FD_SETSIZE; i++)
            {
                if (fds_list[i] == true)
                {
                    FD_SET(i, &read_set);
                }
            }
        }
        else // This should cover all the other socket descriptors
        {
            // Should we iterate through our fds_list and check
            // which descriptor is present in the read/write/error sets?
            for (i = 0; i < FD_SETSIZE; i++)
            {   
                // Check if this socket exists
                if (fds_list[i] == true)
                {
                    // If it is ready to read, go for it.
                    if (FD_ISSET(i, &read_set))
                    {
                        printf("FD: %d\n", i);

						// Let us serve it.
                        ret = serve_connection(i);
                        if (ret == SERVE_CONN_FAILED || ret == SERVE_CONN_CLIENT_DISCONN)
                        {
                            // If something failed, shutdown the client.
                            // If recv returns 0, it means that the other side
                            // has closed the connection. We need to do it
                            // as well.
                            close(i);
                            FD_CLR(i, &read_set);
                            fds_list[i] = false;
                        }

                        // On success, we should do nothing.
                        // Just play along.
                        
						// Because this is an echo server,
						// the client can talk to the server
						// for how much time it wants.
						// We should not be closing the connection.

                        // The descriptor which was just served
                        // is already present in read_set.
                        // Let it be. We want select to monitor it.
                    }
                    else
                    {   
                        // If it is not ready to be read
                        // but is a valid descriptor, then
                        // select should monitor it.
                        FD_SET(i, &read_set);
                    }
                    
                }
            }
        }
    }
}
