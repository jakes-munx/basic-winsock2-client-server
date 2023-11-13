#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>

#include "network.h"

#include <Windows.h>

SOCKET ConnectToServer(char* server_ip, char* server_port)
{
    SOCKET client_socket;
    struct addrinfo hints;
    struct addrinfo *addr_info = NULL;
    struct addrinfo *ptr = NULL;
    
    WSADATA wsaData;

    int result = 0;

    result = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (result != 0) 
    {
        printf("WSAStartup failed. Error: %d\r\n", result);
        return -1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    
    // Resolve the server address and port
    result = getaddrinfo(server_ip, server_port, &hints, &addr_info);
    if ( result != 0 ) 
    {
        printf("getaddrinfo failed with error: %d\n", result);
        WSACleanup();
        return -1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr = addr_info; ptr != NULL; ptr = ptr->ai_next) 
    {

        // Create a SOCKET for connecting to server
        client_socket = socket(ptr->ai_family, ptr->ai_socktype, 
            ptr->ai_protocol);
        if (client_socket == INVALID_SOCKET) 
        {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return -1;
        }

        // Connect to server.
        result = connect( client_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (result == SOCKET_ERROR) 
        {
            closesocket(client_socket);
            printf("Failed to connect to %u:%u:%u:%u\r\n", (uint8_t)ptr->ai_addr->sa_data[2], (uint8_t)ptr->ai_addr->sa_data[3], (uint8_t)ptr->ai_addr->sa_data[4], (uint8_t)ptr->ai_addr->sa_data[5]);
            client_socket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(addr_info);
    
    if (client_socket == INVALID_SOCKET) 
    {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return -1;
    }
    return client_socket;
}

int ProcessNewMessage(SOCKET client_socket, char* p_message)
{
    int recv_buf_len = DEFAULT_BUF_LEN;
    int result = recv(client_socket, p_message, recv_buf_len, 0);
    if (result > 0) 
    {
        p_message[result-1] = 0;    // Null terminate
        printf("Message received: %d bytes\r\n", result);
        // printf("Message: %s\r\n", p_message);
    }
    else if (result == 0)
    {
        printf("Closing socket %llu\r\n", client_socket);
    }
    else  
    {
        printf("Receive failed. Error: %d\r\n", WSAGetLastError());
    }
    return result;
}