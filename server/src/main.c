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
#include <tchar.h>
#include <Windows.h>



#include "network.h"
#include "file_man.h"

int main()
{
    int result;

    printf("Starting server...\r\n");

    // struct sockaddr_in srv;
    // int send_result;
    // char recv_buf[DEFAULT_BUF_LEN];
    // int recv_buf_len = DEFAULT_BUF_LEN;
    char port_number[] = DEFAULT_PORT;

    char user_port[6] = {0};
    printf("Enter server PORT. Press enter to use %s\r\n", DEFAULT_PORT);
    fgets(user_port, sizeof(user_port), stdin);
    user_port[strlen(user_port) - 1] = '\0';    // Remove \n

    if (user_port[0] != 0x00)
    {
        strcpy(port_number, user_port);
    }

    SOCKET listen_socket = SetupListenSocket(port_number);
    
    while (1)
    {
        // Exit on ESC press
        if (GetKeyState(VK_ESCAPE) & 0xffffff80)
        {
            break;
        }

        // Receive any new client activity
        result = AwaitSocketRequest(listen_socket);
        if (result > 0)
        {
            // Accept new clients or process requests from existing ones
            ProcessNewRequest(listen_socket);
        }
        else if (result == 0)
        {
            // printf("Nothing on the port\r\n");
        }
        else
        {
            printf("Socket select error\r\n");
        }
    }

    printf("Shutting down server...\r\n");
    // Cleanup
    
    CloseAllSockets();
    WSACleanup();

    return 0;
}