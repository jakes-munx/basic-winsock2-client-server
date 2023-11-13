#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "file_man.h"
#include "network.h"

static DWORD g_BytesTransferred = 0;

void CALLBACK FileIOCompletionRoutine(
  __in  DWORD dwErrorCode,
  __in  DWORD dwNumberOfBytesTransfered,
  __in  LPOVERLAPPED lpOverlapped )
{
    (void) lpOverlapped;
    if (dwErrorCode != 0)
    {
        printf("Error code:\t%lx\r\n", dwErrorCode);
    }
    printf("Number of file bytes transferred:\t%lu\r\n", dwNumberOfBytesTransfered);
    g_BytesTransferred = dwNumberOfBytesTransfered;
}

uint16_t Hash(char* p_data, int len)
{
    uint16_t hash_val = 0x5555;
    for (int index = 0; index < len; index++)
    {
        hash_val  = (hash_val << 5) ^ (int)p_data[index];
    }
    return hash_val;
}

int SendFile(SOCKET client_socket, char* file_name)
{
    int result = 0;
    char file_data[DEFAULT_BUF_LEN];
    char recv_buf[DEFAULT_BUF_LEN];
    HANDLE hFile; 
    DWORD  dwBytesRead = 0;
    // char   ReadBuffer[DEFAULT_BUF_LEN] = {0};
    OVERLAPPED ol = {0};

    // char cwd[PATH_MAX];
    // if (getcwd(cwd, sizeof(cwd)) != NULL) {
    //     printf("Current working dir: %s\n", cwd);
    // }  

    hFile = CreateFile(file_name,             // file to open
                       GENERIC_READ,          // open for reading
                       FILE_SHARE_READ,       // share for reading
                       NULL,                  // default security
                       OPEN_EXISTING,         // existing file only
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, // normal file
                       NULL);                 // no attr. template

    if (hFile == INVALID_HANDLE_VALUE) 
    { 
        printf("CreateFile: unable to open file \"%s\" for read.\r\n", file_name);
        char zero_file_size = '0';
        send(client_socket, &zero_file_size, 1, 0 );
        return -1; 
    }
    DWORD file_size_high;
    uint64_t file_size = GetFileSize (hFile, &file_size_high);
    file_size += (uint64_t)(file_size_high) * 0xFFFFFFFF;
    uint64_t file_index = 0;

    printf("Size of file: %d bytes\r\n", file_size);

    char s_file_size[8] = {0};
    sprintf(s_file_size, "%d", file_size);
    // Send size of file
    result = send(client_socket, s_file_size, sizeof(s_file_size), 0 );
    if (result == SOCKET_ERROR) 
    {
        printf("Send failed. Error: %d\r\n", WSAGetLastError());
        return -1;
    }
    printf("Bytes sent: %d\r\n", result);

    while (file_index < file_size)
    {
        ol.Offset = (DWORD)(file_index & 0xFFFFFFFFUL);
        ol.OffsetHigh = (DWORD)((file_index >> 32) & 0xFFFFFFFFUL);
        // Read one character less than the buffer size to save room for
        // the terminating NULL character. 
        if( FALSE == ReadFileEx(hFile, file_data, DEFAULT_BUF_LEN-1, &ol, FileIOCompletionRoutine) )
        {
            printf("ReadFile: Unable to read from file.\n GetLastError=%08x\r\n", GetLastError());
            CloseHandle(hFile);
            return -1;
        }
        SleepEx(5000, TRUE);
        dwBytesRead += g_BytesTransferred;
        // This is the section of code that assumes the file is ANSI text. 
        if ((g_BytesTransferred > 0) && (g_BytesTransferred <= DEFAULT_BUF_LEN-1))
        {
            file_data[g_BytesTransferred]='\0'; // NULL character

            printf("Data read from %s (%d bytes, at index %d)\r\n", file_name, g_BytesTransferred, file_index);
        }
        else if (g_BytesTransferred == 0)
        {
            printf("No data read from file %s\r\n", file_name);
            break;
        }
        else
        {
            printf("\n ** Unexpected value for dwBytesRead **\r\n");
            break;
        }
        file_index += g_BytesTransferred;

        // Hash the file section
        uint16_t hash_val = Hash(file_data, g_BytesTransferred);
        char section_data[10] = {0};
        sprintf(section_data, "%d,%d", hash_val, g_BytesTransferred);
        printf("Sending section data: Hash=%d, bytes=%d\r\n", hash_val, g_BytesTransferred);
        result = send(client_socket, section_data, sizeof(section_data), 0 );
        if (result == SOCKET_ERROR) 
        {
            printf("Send failed. Error: %d\r\n", WSAGetLastError());            
            return -1;
        }
        printf("Bytes sent: %d\r\n", result);

        result = ProcessNewMessage(client_socket, recv_buf);
        if ((result == 1) && (recv_buf[0] == ACK))  // Client confirms it wants this section
        {
            result = send(client_socket, file_data, g_BytesTransferred+1, 0 );
            if (result == SOCKET_ERROR) 
            {
                printf("Send failed. Error: %d\r\n", WSAGetLastError());
                // TODO retry
                
                return -1;
            }
            printf("Bytes sent: %d\r\n", result);
        }
        else if ((result == 1) && (recv_buf[0] == NAK))  // Client confirms it does NOT this section
        {
            printf("Data section skipped\r\n");
            continue;
        }
        else
        {
            printf("Unexpected response\r\n");
        }

        result = ProcessNewMessage(client_socket, recv_buf);
        if ((result == 1) && (recv_buf[0] == ACK))  // Client confirms data was recived correctly
        {
            printf("Client Acknowledged the section\r\n");
        }
        else if ((result == 1) && (recv_buf[0] == NAK))  // Client reports error on this section
        {
            // TODO: retry
            printf("Client reports error on this section\r\n");
        }
        else
        {
            printf("Unexpected response\r\n");
        }
    }
    printf("File transfer done for client socket %d\r\n", client_socket);

    CloseHandle(hFile);
    return dwBytesRead;
}