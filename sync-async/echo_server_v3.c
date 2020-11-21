/*
 * echo_server_v3.c
 * 
 * Uses poll as an event notifier.
 * Can handle any number of clients that hit the server.
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
#include <poll.h>

// A pollfd dynamic array implementation
// In order to support any number of incoming connections,
// we need this.
typedef struct pfds
{   
    // Points to the pollfd array.
    // Is subject to callocs, reallocs.
    struct pollfd   *list;

    // Total capacity of the array.
    uint64_t        capacity;

    // Largest index which has a valid descriptor.
    // The moment max_index == capacity-1, we need
    // to increase the list size.
    uint64_t        max_index;

    // This points to a pollfd instance which does not
    // have a valid descriptor.
    // Note: free_index need not be same as max_index.
    //       Suppose 100 connection requests come in.
    //       max_index will be equal to free_index.
    //       One connection whose socket is in list[5]
    //       closes. Then free_index will point to 5.
    // It is going to point to the smallest free index.
    uint64_t        free_index;
} pfds_t;

// Idea behind the implementation
//
// Suppose we start with a list can hold 10 pollfd instances.
// - [-1, -1, -1, -1, -1, -1, -1, -1, -1, -1]
// - capacity = 10, max_index = 0, free_index = 0
// - Now 5 connections come in.
//      - [3, 4, 5, 6, 7, -1, -1, -1, -1, -1]
//      - max_index = 5, free_index = 5.
//
// - Connection with descriptor (4) close. What do we do?
//      - We need to invalidate its entry in the list.
//      - [3, -1, 5, 6, 7, -1, -1, -1, -1, -1]
//      - max_index = 5, free_index = 1;
// - Note that free_index will always point to the smallest
//   free pollfd instance. Idea is to reuse that memory
//   instead of simply allocating more memory when there
//   is free memory here.
// - Everytime a descriptor is added, free_index will
//   definitely change. max_index might change depending
//   free_index's value.


// Initialize a pfds_t structure.
// Generally a stack allocated pfds_t structure
// is passed.
int pfds_init (pfds_t *pfds)
{
    if (pfds == NULL)
    {
        return -1;
    }

    uint64_t    i = 0;

    memset(pfds, '\0', sizeof(pfds_t));

    // Let us start with 1000 descriptors.
    pfds->list = calloc(1000, sizeof(struct pollfd));
    if (pfds->list == NULL)
    {   
        // No memory. Kill the server.
        printf("calloc() failed\n");
        exit(-1);
    }

    // Update the members
    pfds->capacity = 1000;
    pfds->max_index = 0;
    pfds->free_index = 0;

    // Initialize the list
    for (i = 0; i < pfds->capacity; i++)
    {
        pfds->list[i].fd = -1;
        pfds->list[i].events = 0;
        pfds->list[i].revents = 0;
    }

    // All set.
    return 0;
}

// Adding to this means you are asking poll
// to monitor it.
int pfds_add (pfds_t *pfds, struct pollfd *pfd)
{
    // Basic checks
    if (pfds == NULL || pfd == NULL)
    {
        return -1;
    }

    uint64_t        i = 0;
    struct pollfd   *temp = NULL;

    // General strategy is to copy pfd
    // into the instance pointed by free_index.
    // Note that free_index will always point to
    // a valid pollfd instance.
    pfds->list[pfds->free_index].fd = pfd->fd;
    pfds->list[pfds->free_index].events = pfd->events;
    pfds->list[pfds->free_index].revents = pfd->revents;

    // If we just populated the instance pointed by
    // max_index+1, then update max_index.
    if (pfds->free_index == pfds->max_index + 1)
    {
        pfds->max_index += 1;
    }

    // Now, our job is to find out the next free_index.
    // The new free_index will be used by the next add()

    // There are two cases:

    // 1. If the current free_index is found before max_index,
    //    - There might be a usable pollfd instance before
    //      max_index.
    if (pfds->free_index < pfds->max_index)
    {
        for(i = pfds->free_index+1; i <= pfds->max_index; i++)
        {   
            // If there is a usable instance?
            if (pfds->list[i].fd == -1)
            {   
                // Assign it. We are all set 
                // for the next add().
                pfds->free_index = i;
                return 0;
            }
        }
    }                  

    // 2. We are here means there are no usable pollfd instances
    // till max_index (including it).
    // - This simply means that the next free_index is
    //   max_index+1.
    // - What if max_index points to the last pollfd instance?
    if (pfds->max_index == pfds->capacity - 1)
    {
        // This means that there is no memory for the next
        // add(). Let us allocate some.
        temp = realloc(pfds->list, sizeof(struct pollfd) * (pfds->capacity + 100));
        if (temp == NULL)
        {
            // This means no memory - system may not be doing good.
            // kill the server.
            printf("realloc() failed. Exiting...\n");
            exit(-1);
        }

        // Let us update the members.
        pfds->list = temp;
        pfds->capacity += 100;

        // Let us initialize the 100 new instances.
        for (i = pfds->max_index+1; i < pfds->capacity; i++)
        {
            pfds->list[i].fd = -1;
            pfds->list[i].events = 0;
            pfds->list[i].revents = 0;
        }
    }

    // At this point, max_index+1 points to a valid pollfd instance.
    // We can set the next free_index.
    pfds->free_index = pfds->max_index+1;

    // Go to go.
    return 0;
}

// Remove a descriptor from monitoring.
int pfds_remove (pfds_t *pfds, uint64_t index)
{
    // Basic checks
    if (pfds == NULL)
    {
        return -1;
    }

    // We need to ensure index is well within limits.
    if (index > pfds->max_index)
    {
        return -1;
    }

    // Make sure it is a valid descriptor.
    if (pfds->list[index].fd == -1)
    {
        return -1;
    }

    // Clean up.
    close(pfds->list[index].fd);
    pfds->list[index].fd = -1;
    pfds->list[index].events = 0;
    pfds->list[index].revents = 0;

    // We might need to update max_index and free_index
    // based on index.
    if (index == pfds->max_index)
    {
        // If free_index points to
        // pfds->max_index+1, then it should
        // be updated. Because pfds->max_index
        // just got cleaned up.
        if (pfds->free_index == pfds->max_index+1)
        {
            pfds->free_index -= 1;
        }
        
        // If the last instance was just cleaned up,
        // that is not the last instance anymore.
        pfds->max_index -= 1;
    }
    else
    {
        // This is the case where index is somewhere
        // in the middle of the array.
        // Here, there is a possibility of index
        // being less than free_index. Meaning,
        // index can be the new free_index.
        if (index < pfds->free_index)
        {
            pfds->free_index = index;
        }
    }

    // Good to go.
    return 0;
}

// serve_connection can have different return values.
// Based on it, we need to take action in the main
// function.
enum 
{
    SERVE_CONN_SUCCESS = 0,
    SERVE_CONN_FAILED,
    SERVE_CONN_CLIENT_DISCONN,
};

int serve_connection (int client_fd)
{   
    uint8_t         request_buffer[10000] = {0};
    int             ret = 0;
    int             req_len = 0;

    printf("serve_connection on descriptor %d invoked\n", client_fd);

    // Get the data.
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
        // XXX: We should never hit this case
        //      because POLLHUP should take care of this.
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

    printf("serve_connection on descriptor %d done\n", client_fd);
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
    pfds_t              pfds = {0};
    struct pollfd       pfd = {0};
    int                 ready_fd_count = 0;

    // Initialize pfds
    ret = pfds_init(&pfds);
    if (ret < 0)
    {
        printf("pfds_init() failed\n");
        return -1;              
    }

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

    // Add the server socket.
    pfd.fd = sock_fd;
    pfd.events |= POLLIN;
    pfd.revents = 0;
    ret = pfds_add(&pfds, &pfd);
    if (ret < 0)
    {
        printf("pfds_add() failed\n");
        return -1;
    }

    // Do the thing
    while (1)
    {   
        ret = poll(pfds.list, pfds.max_index+1 /* Number of descriptors */, -1 /* Infinite timeout */);
        if (ret < 0)
        {
            printf("poll() failed\n");
            return -1;
        }
        ready_fd_count = ret;
        printf("No of ready descriptors: %d\n", ready_fd_count);

        // All server socket related things first.
        // Check for errors.
        if (pfds.list[0].revents & POLLERR)
        {
            // Some error occured while monitoring the server socket.
            // Let us kill the server.
            printf("poll() error(POLLERR) on server descriptor. Exiting...");
            exit(-1);
        }
        else if (pfds.list[0].revents & POLLIN)
        {
            // If it is ready to be read (in other words, there are
            // new connection requests, process it.)
            memset(&client_addr, '\0', sizeof(client_addr));
            ret = accept(pfds.list[0].fd, (struct sockaddr *)&client_addr, &client_addr_len);
            printf("accept() returned %d\n", ret);
            if (ret < 0)
            {
                printf("accept() failed\n");
                return -1;
            }
            client_fd = ret;
            
            // We have a new socket descriptor. Let us add it.
            memset(&pfd, '\0', sizeof(struct pollfd));
            pfd.fd = client_fd;
            pfd.events |= POLLIN;
            ret = pfds_add(&pfds, &pfd);
            if (ret < 0)
            {
                printf("pfds_add() failed\n");
                return -1;
            }
        }

        // All server related things are done.
        // Onto the clients.
        // Iterate through the pfds list.
        for (i = 1; i <= pfds.max_index; i++)
        {   
            // First make sure it is a valid descriptor.
            if (pfds.list[i].fd != -1)
            {   
                // Check for error or if client closed connection.
                if (pfds.list[i].revents & POLLERR || pfds.list[i].revents & POLLHUP)
                {   
                    printf("Removing descriptor %d\n", pfds.list[i].fd);
                    pfds_remove(&pfds, i);
                }
                // Let us check if it is ready for reading.
                else if (pfds.list[i].revents & POLLIN)
                {
                    // Let us serve the connection.
                    ret = serve_connection(pfds.list[i].fd);
                    if (ret != SERVE_CONN_SUCCESS)
                    {
                        pfds_remove(&pfds, i);
                    }
                }
            }
        }
    }
}