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

const char short_quit_cmd = 'q';
const char long_quit_cmd[] = "quit";

int main()
{
    int result;
    SOCKET client_socket;

    char recv_buf[DEFAULT_BUF_LEN];
    char send_buf[DEFAULT_BUF_LEN];
    int send_buf_len = DEFAULT_BUF_LEN;

    char user_ip[20] = {0};
    char user_port[6] = {0};
    printf("Enter IP address of server (with colons). Press enter to use %s\r\n", DEFAULT_SERVER_IP);
    fgets(user_ip, sizeof(user_ip), stdin);
    user_ip[strlen(user_ip) - 1] = 0;    // Remove \n
    printf("Enter server PORT. Press enter to use %s\r\n", DEFAULT_PORT);
    fgets(user_port, sizeof(user_port), stdin);
    user_port[strlen(user_port) - 1] = 0;    // Remove \n

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

    client_socket = ConnectToServer(host_name, port_number);

    // Receive first message  
    result = ProcessNewMessage(client_socket, recv_buf);
    if (result < 0)
    {
        closesocket(client_socket);
        WSACleanup();
        return -1;
    }

    while (1)
    {
        int cmd_len;
        printf("Enter file name to retrieve. Type 'q' to exit\r\n");
        fgets(send_buf, send_buf_len, stdin);
        cmd_len = strlen(send_buf);
        send_buf[cmd_len - 1] = 0;    // Replace \n with NULL terminator.

        // If user enters 'q' or "quit", exit while loop and shutdown gracefully
        if ( ((cmd_len == 2) && ((send_buf[0] == 'q') || (send_buf[0] == 'Q')))
            || ((cmd_len == sizeof(long_quit_cmd)) && (strncasecmp(send_buf, long_quit_cmd, cmd_len) == 0)) )
        {
            break;
        }
        // Request the file entered
        GetFile(client_socket, send_buf, cmd_len);    
    }   

    // shutdown the connection since no more data will be sent
    result = shutdown(client_socket, SD_SEND);
    if (result == SOCKET_ERROR) 
    {
        printf("shutdown failed with error: %d\r\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection
    do 
    {
        result = ProcessNewMessage(client_socket, recv_buf);
    } while( result > 0 );

    // cleanup
    closesocket(client_socket);
    WSACleanup();

    return 0;
}