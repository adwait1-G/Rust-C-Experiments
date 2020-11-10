/*
 * server_v1.c
 * 
 * Server which can serve multiple connections at a time, using threads.
 * Blocking is still an issue.
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
#include <pthread.h>
#include <sys/syscall.h>

pid_t gettid()
{
    return syscall(0xba);
}

void* serve_connection (void *client_fd)
{   
    uint8_t         request_buffer[10000] = {0};
    uint8_t         response_buffer[10000] = {0};
    int             ret = 0;
    int             fd = *(int *)client_fd;

    printf("Serving client with fd: %d using thread with tid = %d, pid = %d\n", fd, gettid(), getpid());

    // Only one request response!
    ret = recv(fd, request_buffer, sizeof(request_buffer), 0);
    if (ret < 0)
    {
        printf("recv() failed for fd = %d\n", fd);
        return NULL;
    }

    // Print the request (symbolic of processing the request)
    printf("%d: %s\n", fd, request_buffer);

    // Send back response
    ret = send(fd, "Hello from server!", 19, 0);
    if (ret < 19)
    {
        printf("send() failed for fd = %d\n", fd);
        return NULL;
    }

    // Close up the socket.
    close(fd);

    // Free up the memory for the file descriptor.
    free(client_fd);

    return NULL;
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
    int                 *arg_client_fd = NULL;
    int                 ret = 0;
    int                 i = 0;
    struct sockaddr_in  server_addr = {0};
    struct sockaddr_in  client_addr = {0};
    socklen_t           client_addr_len = 0;
    const char          *ip_addr = argv[1];
    uint16_t            port_no = atoi(argv[2]);
    pthread_attr_t      attr;
    pthread_t           tinfo;


    // Initialize the pthread creation attributes
    ret = pthread_attr_init(&attr);
    if (ret != 0)
    {
        printf("pthread_attr_init() failed\n");
        return -1;
    }

    // Socket related work.
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

        // Create a thread here.
        // Make a copy of the client_fd.
        arg_client_fd = calloc(1, sizeof(int));
        if (arg_client_fd == NULL)
        {
            printf("calloc() failed\n");
            return -1;
        }

        *arg_client_fd = client_fd;
        ret = pthread_create(&tinfo, &attr, serve_connection, arg_client_fd);
    }
}
