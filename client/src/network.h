#ifndef __NETWORK_H
#define __NETWORK_H

#include <WinSock2.h>
#include <WS2spi.h>
#include <WS2tcpip.h>

#define DEFAULT_BUF_LEN 512
#define DEFAULT_PORT "1234"
#define DEFAULT_SERVER_IP "127.0.0.1"

SOCKET ConnectToServer(char* server_ip, char* server_port);
int ProcessNewMessage(SOCKET client_socket, char* p_message);

#endif