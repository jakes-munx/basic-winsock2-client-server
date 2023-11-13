#ifndef __NETWORK_H
#define __NETWORK_H

#include <WinSock2.h>
#include <WS2spi.h>
#include <WS2tcpip.h>

#define DEFAULT_BUF_LEN 512
#define DEFAULT_PORT "1234"
#define MAX_CLIENTS 10

#define ACK 6
#define NAK 21

void CloseClientSocket(SOCKET client_socket);
void CloseAllSockets(void);
void ProcessNewRequest(SOCKET listen_socket);
SOCKET SetupListenSocket(char* port);
int ProcessNewMessage(SOCKET client_socket, char* p_message);
int AwaitSocketRequest(SOCKET listen_socket);

#endif