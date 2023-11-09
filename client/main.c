#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
#define DEFAULT_PORT "8080"
#define DEFAULT_HOST "127.0.0.1"

int main()
{
    WSADATA wsaData;
    int result;

    SOCKET client_socket;
    // struct sockaddr_in srv;

    struct addrinfo *addr_info = NULL;
    struct addrinfo *ptr = NULL;
    struct addrinfo hints;

    const char *sendbuf = "BOOM goes the dynamite!";
    char recv_buf[DEFAULT_BUF_LEN];
    int recv_buf_len = DEFAULT_BUF_LEN;

    result = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (result != 0) 
    {
        printf("WSAStartup failed. Error: %d\r\n", result);
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char host_name[] = DEFAULT_HOST;
    char port_number[] = DEFAULT_PORT;
    // Resolve the server address and port
    result = getaddrinfo(host_name, port_number, &hints, &addr_info);
    if ( result != 0 ) 
    {
        printf("getaddrinfo failed with error: %d\n", result);
        WSACleanup();
        return 1;
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
            return 1;
        }

        // Connect to server.
        result = connect( client_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (result == SOCKET_ERROR) 
        {
            closesocket(client_socket);
            printf("Failed to connect to %u:%u:%u:%u:%u:%u\r\n", (uint8_t)ptr->ai_addr->sa_data[0], (uint8_t)ptr->ai_addr->sa_data[1], (uint8_t)ptr->ai_addr->sa_data[2], (uint8_t)ptr->ai_addr->sa_data[3], (uint8_t)ptr->ai_addr->sa_data[4], (uint8_t)ptr->ai_addr->sa_data[5]);
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
        return 1;
    }

    // Send an initial buffer
    result = send( client_socket, sendbuf, (int)strlen(sendbuf), 0 );
    if (result == SOCKET_ERROR) 
    {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %ld\n", result);

    // shutdown the connection since no more data will be sent
    result = shutdown(client_socket, SD_SEND);
    if (result == SOCKET_ERROR) 
    {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection
    do {

        result = recv(client_socket, recv_buf, recv_buf_len, 0);
        if ( result > 0 )
        {
            printf("Bytes received: %d\n", result);
            printf("Message: %s\r\n", recv_buf);
        }
        else if ( result == 0 )
        {
            printf("Connection closed\n");
        }
        else
        {
            printf("recv failed with error: %d\n", WSAGetLastError());
        }

    } while( result > 0 );

    // cleanup
    closesocket(client_socket);
    WSACleanup();

    return 0;
}