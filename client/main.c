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
#define DEFAULT_PORT "1234"
#define DEFAULT_SERVER_IP "127.0.0.1"

SOCKET ConnectToServer(char* server_ip, char* server_port)
{
    SOCKET client_socket;
    struct addrinfo hints;
    struct addrinfo *addr_info = NULL;
    struct addrinfo *ptr = NULL;
    
    int result = 0;

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
        return 1;
    }
    return client_socket;
}
int main()
{
    WSADATA wsaData;
    int result;
    SOCKET client_socket;
    // struct sockaddr_in srv;

    const char *sendbuf = "BOOM goes the dynamite!";
    char recv_buf[DEFAULT_BUF_LEN];
    int recv_buf_len = DEFAULT_BUF_LEN;

    char user_ip[20] = {0};
    char user_port[6] = {0};
    printf("Enter IP address of server (with colons). Press enter to use %s\r\n", DEFAULT_SERVER_IP);
    fgets(user_ip, sizeof(user_ip), stdin);
    user_ip[strlen(user_ip) - 1] = '\0';    // Remove \n
    printf("Enter server PORT. Press enter to use %s\r\n", DEFAULT_PORT);
    fgets(user_port, sizeof(user_port), stdin);
    user_port[strlen(user_port) - 1] = '\0';    // Remove \n

    char host_name[20] = DEFAULT_SERVER_IP;
    char port_number[5] = DEFAULT_PORT;

    if (user_ip[0] != 0x00)
    {
        memset(host_name, 0, sizeof(host_name));
        strcpy(host_name, user_ip);
    }
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

    client_socket = ConnectToServer(host_name, port_number);

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