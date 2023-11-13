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
#include "file_man.h"

#include <Windows.h>

static SOCKET arr_clients[MAX_CLIENTS];
fd_set fr, fw, fe;
uint8_t max_fd;
struct timeval tv = {0, 1};

void CloseClientSocket(SOCKET client_socket)
{
    printf("Closing socket %d\r\n", client_socket);
    closesocket(client_socket);
    for (uint8_t socket_index = 0; socket_index < MAX_CLIENTS; socket_index++)
    {
        if (arr_clients[socket_index] == client_socket)
        {
            arr_clients[socket_index] = 0; // Clear the socket from the array
            break;
        }
    }
}

void CloseAllSockets(void)
{
    for (uint8_t socket_index = 0; socket_index < MAX_CLIENTS; socket_index++)
    {
        if (arr_clients[socket_index] != 0)
        {
            arr_clients[socket_index] = 0; // Clear the socket from the array
            closesocket(arr_clients[socket_index]);
        }
    }
}

void ProcessNewRequest(SOCKET listen_socket)
{
    int result;

    char accept_msg[] = "Connection accepted";
    if (FD_ISSET(listen_socket, &fr))   //Service listener requests for new clients
    {
        SOCKET client_socket = accept(listen_socket, NULL, NULL);
        if (client_socket != INVALID_SOCKET)
        {
            for (uint8_t socket_index = 0; socket_index < MAX_CLIENTS; socket_index++)
            {
                if (arr_clients[socket_index] == 0) // Empty client slot
                {
                    arr_clients[socket_index] = client_socket;
                    printf("New client added on socket %d\r\n", client_socket);
                    send(arr_clients[socket_index], accept_msg, sizeof(accept_msg), 0);
                    break;
                }
                if (socket_index == MAX_CLIENTS - 1)
                {
                    printf("No capacity for new client\r\n");
                }
            }

        }
    }
    else    //Service requests from existing clients
    {
        for (uint8_t socket_index = 0; socket_index < MAX_CLIENTS; socket_index++)
        {
            if (FD_ISSET(arr_clients[socket_index], &fr))
            {
                char recv_buf[DEFAULT_BUF_LEN];
                if (ProcessNewMessage(arr_clients[socket_index], recv_buf) > 0)
                {
                    printf("Client on socket %d has requested \"%s\"\r\n", arr_clients[socket_index], recv_buf);
                    SendFile(arr_clients[socket_index], recv_buf);
                }
                break;
            }
        }
    }
}

SOCKET SetupListenSocket(char* port)
{
    int result;
    WSADATA wsaData;

    SOCKET listen_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;

    struct addrinfo *addr_info = NULL;
    struct addrinfo hints;

    result = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (result != 0) 
    {
        printf("WSAStartup failed. Error: %d\r\n", result);
        return 1;
    }
    printf("WSA startup successful\r\n");

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    //  Resolve the server address and port
    // getaddrinfo(argv[1], argv[2], &hints, &addr_info);
    result = getaddrinfo(NULL, port, &hints, &addr_info);
    if ( result != 0 ) 
    {
        printf("Getaddrinfo failed. Error: %d\r\n", result);
        WSACleanup();
        return -1;

    }
    // Init the socket
    listen_socket = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol);
    if (listen_socket == INVALID_SOCKET) {
        printf("Socket failed: Error: %ld\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }
    printf("Listening Socket created: %d\r\n", (int)listen_socket);

    // srv.sin_family = AF_INET;
    // srv.sin_port = htons(8080);
    // srv.sin_addr.s_addr = INADDR_ANY;
    // memset(&(srv.sin_zero), 0, sizeof(srv.sin_zero));

    // result = bind(listen_socket, (struct sockaddr*) &srv, sizeof(struct sockaddr));
    result = bind(listen_socket, addr_info->ai_addr, (int)addr_info->ai_addrlen);
    if (result == SOCKET_ERROR) 
    {
        printf("Bind failed. Error: %d\r\n", WSAGetLastError());
        freeaddrinfo(addr_info);
        closesocket(listen_socket);
        WSACleanup();
        return -1;
    }
    printf("Socket binding successful\r\n");

    freeaddrinfo(addr_info);

    result = listen(listen_socket, MAX_CLIENTS);
    if (result == SOCKET_ERROR) 
    {
        printf("Listen failed. Error: %d\r\n", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return -1;
    }
    printf("Listening for clients...\r\n");

    return listen_socket;
}

int ProcessNewMessage(SOCKET client_socket, char* p_message)
{
    int recv_buf_len = DEFAULT_BUF_LEN;
    int result = recv(client_socket, p_message, recv_buf_len, 0);
    if (result > 0) 
    {
        printf("Message received: %d bytes\r\n", result);
        // printf("Message: %s\r\n", p_message);

    }
    else if (result == 0)
    {
        CloseClientSocket(client_socket);
    }
    else  
    {
        printf("Receive failed. Error: %d\r\n", WSAGetLastError());
        CloseClientSocket(client_socket);
        return -1;
    }
    return result;
}

int AwaitSocketRequest(SOCKET listen_socket)
{
    int result;

    FD_ZERO(&fr);
    FD_ZERO(&fw);
    FD_ZERO(&fe);

    FD_SET(listen_socket, &fr);
    FD_SET(listen_socket, &fe);
    
    for (uint8_t socket_index = 0; socket_index < MAX_CLIENTS; socket_index++)
    {
        if (arr_clients[socket_index] != 0)
        {
            FD_SET(arr_clients[socket_index], &fr);
            FD_SET(arr_clients[socket_index], &fe);
        }
    }

    result = select((listen_socket + 1), &fr, &fw, &fe, &tv);
}