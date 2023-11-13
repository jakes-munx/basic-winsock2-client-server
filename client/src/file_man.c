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

#include "file_man.h"

#include <Windows.h>

#define ACK 6
#define NAK 21

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

int GetFile(SOCKET client_socket, char* p_file_name, int file_name_len)
{
    char recv_buf[DEFAULT_BUF_LEN];
    int file_size = 0;
    int hash_rec = 0;
    int section_size_rec = 0;
    int result = send(client_socket, p_file_name, file_name_len, 0);
    if (result == SOCKET_ERROR) 
    {
        printf("send failed with error: %d\r\n", WSAGetLastError());
        return -1;
    }
    printf("Bytes Sent: %d\n", result);

    //Get file size
    result = ProcessNewMessage(client_socket, recv_buf);
    if (result > 0)
    {
        file_size = atoi(recv_buf);
        if (file_size == 0)
        {
            printf("File does not exist\r\n");
            return 0;
        }
        else
        {
            printf("file exists with size %d bytes\r\n", file_size);
        }
    }
    else
    {
        return -1;
    }

    DWORD dwAttrib = GetFileAttributes(p_file_name);
    bool file_exists_locally = false;
    if ((dwAttrib == INVALID_FILE_ATTRIBUTES) || (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        printf("File does not exist locally.\r\n");
    }
    else
    {
        file_exists_locally = true;
    }

    HANDLE hFile = CreateFile(p_file_name,    // file to open
                       GENERIC_ALL,           // open for rtead/write
                       FILE_SHARE_READ | FILE_SHARE_WRITE, // share for read/write
                       NULL,                  // default security
                       OPEN_ALWAYS,            // create if non-existing
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, // normal file
                       NULL);                 // no attr. template

    if (hFile == INVALID_HANDLE_VALUE) 
    { 
        printf("CreateFile: unable to open/create file \"%s\".\r\n", p_file_name);
        return -1; 
    }

    DWORD file_size_high;
    uint64_t local_file_size = GetFileSize (hFile, &file_size_high);
    local_file_size += (uint64_t)(file_size_high) * 0xFFFFFFFF;
    uint64_t file_index = 0;
    OVERLAPPED ol = {0};
    DWORD  dwBytesWritten = 0;
    while (file_index < file_size)
    {
        // Get section hash
        result = ProcessNewMessage(client_socket, recv_buf);
        if (result > 0)
        {
            char* sep_addr = strchr(recv_buf, ',');   
            if (sep_addr == NULL)
            {
                printf("Hash messaged not received properly\r\n");
            }
            else
            {
                size_t sep_pos = (unsigned)(sep_addr - &recv_buf[0]);    // Address of seperator minus address of array gives the index
                char s_hash_rec[6] = {0};
                char s_section_size_rec[5] = {0};
                
                memcpy(s_hash_rec, &recv_buf[0], sep_pos);
                memcpy(s_section_size_rec, &recv_buf[sep_pos+1], (size_t)(result - sep_pos -1));
                hash_rec = atoi(s_hash_rec);
                section_size_rec = atoi(s_section_size_rec);
                printf("Received hash: %d, section size: %d bytes\r\n", hash_rec, section_size_rec);
            }
        }
        else
        {
            CloseHandle(hFile);
            return -1;
        }

        ol.Offset = (DWORD)(file_index & 0xFFFFFFFFUL);
        ol.OffsetHigh = (DWORD)((file_index >> 32) & 0xFFFFFFFFUL);
        bool ack_file_section;
        // Only bother checking the hash if we had a copy of the file, and it is as long as the current section
        if ((file_exists_locally) && (file_index < local_file_size))    
        {
            // Read the section
            DWORD dwBytesRead = 0;
            char file_data[DEFAULT_BUF_LEN] = {0};
            if( FALSE == ReadFileEx(hFile, file_data, DEFAULT_BUF_LEN-1, &ol, FileIOCompletionRoutine) )
            {
                printf("ReadFile: Unable to read from file.\n GetLastError=%08x\r\n", GetLastError());
                CloseHandle(hFile);
                return -1;
            }
            SleepEx(5000, TRUE);
            dwBytesRead += g_BytesTransferred;

            uint16_t local_hash = Hash(file_data, dwBytesRead);
            ack_file_section = (local_hash != hash_rec);    // ACK the request if our version does not match
        }
        else
        {
            ack_file_section = true;
        }

        if (ack_file_section)
        {   
            // ACK the server to send this section
            char ack = ACK;
            int result = send(client_socket, &ack, 1, 0);
            if (result == SOCKET_ERROR) 
            {
                printf("send failed with error: %d\r\n", WSAGetLastError());
                CloseHandle(hFile);
                return -1;
                // break;
            }
            printf("ACK Sent: %d\n", result);

            // Get file section
            result = ProcessNewMessage(client_socket, recv_buf);
            if (result > 0)
            {
                int section_size  = result - 1; // Ignore NULL terminator
                uint16_t hash_calc = Hash(recv_buf, section_size);
                printf("Calculated hash: %d\r\n", hash_calc);
                if (hash_calc != hash_rec)
                {
                    char nak = NAK;
                    result = send(client_socket, &nak, 1, 0);
                    if (result == SOCKET_ERROR) 
                    {
                        printf("send failed with error: %d\r\n", WSAGetLastError());
                        CloseHandle(hFile);
                        return -1;
                        // break;
                    }
                    printf("NAK Sent: %d\n", result);

                }
                else
                {
                    if( FALSE == WriteFileEx(hFile, recv_buf, (DWORD)section_size, &ol, FileIOCompletionRoutine) )  // Write received data directly to file, at current index
                    {
                        printf("ReadFile: Unable to read from file.\n GetLastError=%08lx\r\n", GetLastError());
                        CloseHandle(hFile);
                        return -1;
                    }
                    SleepEx(5000, TRUE);
                    dwBytesWritten += g_BytesTransferred;

                    char ack = ACK;
                    int result = send(client_socket, &ack, 1, 0);
                    if (result == SOCKET_ERROR) 
                    {
                        printf("send failed with error: %d\r\n", WSAGetLastError());
                        CloseHandle(hFile);
                        return -1;
                        // break;
                    }
                    printf("ACK Sent: %ld\n", result);
                }
            }
            else
            {
                CloseHandle(hFile);
                return -1;
            }
            
        }
        else
        {
            // NAK the server to skip this section, as our version is the same
            char nak = NAK;
            int result = send(client_socket, &nak, 1, 0);
            if (result == SOCKET_ERROR) 
            {
                printf("send failed with error: %d\r\n", WSAGetLastError());
                CloseHandle(hFile);
                return -1;
                // break;
            }
            printf("NAK Sent: %ld\n", result);
        }

        file_index += section_size_rec;
    }
    CloseHandle(hFile);
    printf("Bytes written to %s: %d\r\n", p_file_name, dwBytesWritten);
    return dwBytesWritten;
}