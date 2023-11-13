#ifndef __FILE_MAN_H
#define __FILE_MAN_H

#include "network.h"

int GetFile(SOCKET client_socket, char* p_file_name, int file_name_len);

#endif