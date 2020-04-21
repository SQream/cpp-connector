// ---------------------------------------------------------------------------------
//
// File:    SOCK_CLI.HPP
//
//
// Notes:   Socket client class definition
//          Main file           - SOCK_CLI.CPP    
//          Inline functions    - SOCK_CLI.HXX
//          Message             - --SIGNATURE-- content --SIGNATURE--
//          Signature is defined as MSG_SOCK_SIGN constant
//
//			This class is used by our driver via XMLNode class to
//			stream a request to the server.
//
// ----------------------------------------------------------------------------------


#ifndef __linux__
#pragma comment(lib, "Ws2_32.lib")
#include <windows.h>
typedef int64_t sock_buf_size;
typedef int64_t ssize_t;
#else
typedef size_t sock_buf_size;
#endif
#include <stdlib.h>
#include <string.h>
#ifndef _SOCK_CLI_HPP
#define _SOCK_CLI_HPP

#ifdef __linux__
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>

#define PHOSTENT hostent*
#define SOCKET unsigned int
#define INVALID_SOCKET  (SOCKET)(~0)
#define h_addr h_addr_list[0]
#define SOCKET_ERROR            (-1)

#endif
// ---------------------------- defines -------------------------------
#define DEFAULT_PORT                9999                            // default port to use 

#define MSG_SOCK_SIGN               "___\x4\x4MSG_SOCK\x4\x4___"    // socket msg signature
#define MSG_SOCK_SIGN_SIZE          18                              // length of signature

#define DEF_CHUNK_SIZE              4096                            // default chunk size for reading a full msg

// ----------------------- client socket class ------------------------
struct ssl_st;
typedef struct ssl_st SSL;

class TSocketClient
{

public:

        // member functions
	    std::tuple<int, std::string, bool>            GetParamsFromPicker(void);

        bool            Initialize              ( const char* pServer, int pPort  );
        bool            SockCreateAndConnect    ( void );                                                                 // create and connect via socket    
        bool            SockWriteChunk          ( const void* pBuffer, size_t pChunkSize, int& pBytesWritten );              // write a chunk to socket
        bool            SockReadChunk           ( char* pBuffer, int& pBytesRead, int pChunkSize );                       // read a chunk from socket 
        bool            SockReadFullMsg         ( char*& pBuffer, int& pMsgSize, int pDefChunkSize = DEF_CHUNK_SIZE );    // read the complete msg 
        bool            SockReadHTTPResponse    ( char*& pBuffer, int& pMsgSize, int pDefChunkSize = DEF_CHUNK_SIZE );    // read the complete HTTP response

        bool            WriteSignature          ( void );           // writes the start signature

        void            SockClose               ( void );           // closes the existing open socket if any    

        // common functions

inline  const char*     GetErrMsg               ( void );           // get the last error that occured
inline  bool            IsReady                 ( void );           // check if class is initialized properly

        // constructors - destructor

        TSocketClient   ( bool is_ssl_ );                            // default constructor    
        TSocketClient   ( const char* pServer, int pPort , bool is_ssl_ );              // constructor
        ~TSocketClient  ();                                         // destructor       

private:

        // data members

        SOCKET          vSocket;                        // socket handle
        struct sockaddr_in  vSockAddr;                      // sockaddr details
        SSL *ssl;
        bool is_ssl;
        // other data members

        char         vszErrMsg[256];                     // last error msg

        
        // private function members

        bool    SetErrMsg               ( bool flgIncludeWin32Error, const char* pszErrMsg, ... );

        void ShutdownSSL();
		ssize_t sock_send(const char *__buf, sock_buf_size __n);
		ssize_t sock_recv(char *__buf, sock_buf_size __n);
        
public:

        // static data members

static  bool        g_flgLibReady;        

        // static member functions

static  int     SockInitLib             ( short pMajorVersion = 1, short pMinorVersion = 1 );
static  bool    SockFinalizeLib         ( void );
};

// ---------------------------- include inline functions ----------------------
#include "socket.hxx"
#endif
