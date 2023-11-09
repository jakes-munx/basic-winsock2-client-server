#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <WinSock2.h>
#include <WS2spi.h>
#include <WS2tcpip.h>
#include <Windows.h>

#define DEFAULT_BUF_LEN 512
#define DEFAULT_PORT "1234"
#define MAX_CLIENTS 10

SOCKET SetupSocket(char* port)
{
    int result;
    SOCKET listen_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;

    struct addrinfo *addr_info = NULL;
    struct addrinfo hints;

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
        return 1;
    }
    // Init the socket
    listen_socket = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol);
    // listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET) {
        printf("Socket failed: Error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
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
        return 1;
    }
    printf("Socket binding successful\r\n");

    freeaddrinfo(addr_info);

    result = listen(listen_socket, MAX_CLIENTS);
    if (result == SOCKET_ERROR) 
    {
        printf("Listen failed. Error: %d\r\n", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }
    printf("Listening for clients...\r\n");


    client_socket = accept(listen_socket, NULL, NULL);
    if (client_socket == INVALID_SOCKET) {
        printf("Accept failed. Error: %d\r\n", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }
    printf("Client accepted\r\n");

    // Listen Socket no longer needed.
    closesocket(listen_socket);

    return client_socket;
}

int main()
{
    WSADATA wsaData;
    int result;

    printf("Starting server...\r\n");

    // struct sockaddr_in srv;
    int send_result;
    char recv_buf[DEFAULT_BUF_LEN];
    int recv_buf_len = DEFAULT_BUF_LEN;
    char port_number[] = DEFAULT_PORT;

    char user_port[6] = {0};
    printf("Enter server PORT. Press enter to use %s\r\n", DEFAULT_PORT);
    fgets(user_port, sizeof(user_port), stdin);
    user_port[strlen(user_port) - 1] = '\0';    // Remove \n

    if (user_port[0] != 0x00)
    {
        strcpy(port_number, user_port);
    }

    result = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (result != 0) 
    {
        printf("WSAStartup failed. Error: %d\r\n", result);
        return 1;
    }
    printf("WSA startup successful\r\n");
    
    SOCKET client_socket = SetupSocket(port_number);
    
    printf("Waiting for data...\r\n");
    
    do
    {
        result = recv(client_socket, recv_buf, recv_buf_len, 0);
        if (result > 0) 
        {
            printf("Message received: %d bytes\r\n", result);
            printf("Message: %s\r\n", recv_buf);

            // Reply to the sender with an echo
            send_result = send( client_socket, recv_buf, result, 0 );
            if (send_result == SOCKET_ERROR) 
            {
                printf("Send failed. Error: %d\n", WSAGetLastError());
                closesocket(client_socket);
                WSACleanup();
                return 1;
            }
            printf("Bytes sent: %d\r\n", send_result);
        }
        else if (result == 0)
        {
            printf("Connection closing...\r\n");
        }
        else  
        {
            printf("Receive failed. Error: %d\r\n", WSAGetLastError());
            closesocket(client_socket);
            WSACleanup();
            return 1;
        }
    }
    while (result > 0);

    result = shutdown(client_socket, SD_SEND);
    if (result == SOCKET_ERROR) 
    {
        printf("Shutdown failed. Error: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    // Cleanup
    closesocket(client_socket);
    WSACleanup();

    return 0;
}