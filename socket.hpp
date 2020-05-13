#include <stdio.h>
#include <string>
#include <tuple>
#include <memory>
#ifndef __linux__
#include <winsock.h>
#endif


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

struct TSocketClient {

public:

        bool            Initialize              ( const char* pServer, int pPort  );
        bool            SockCreateAndConnect    ( void );                                                                 // create and connect via socket    
        bool            SockWriteChunk          ( const void* pBuffer, size_t pChunkSize, int& pBytesWritten );              // write a chunk to socket
        bool            SockReadChunk           ( char* pBuffer, int& pBytesRead, int pChunkSize );                       // read a chunk from socket 
        void            SockClose               ( void );           // closes the existing open socket if any    

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


#endif   // of hpp


#include <errno.h>
#include <openssl/ssl.h>

//#include <openssl/err.h>
//#include <openssl/ossl_typ.h>

ssize_t TSocketClient::sock_recv (char *__buf, sock_buf_size __n) {

    return (is_ssl) ? SSL_read(ssl, __buf, __n) : recv(vSocket, __buf, __n, 0);
}

ssize_t TSocketClient::sock_send(const char *__buf, sock_buf_size __n) {

        return (is_ssl) ? SSL_write(ssl, __buf, __n) : send(vSocket, __buf, __n, 0);
}

// ----------------- static data members initialization ------------------
bool TSocketClient::g_flgLibReady = 0;

// --------------------------------------------------------------------
// does the work of the constructor, initializes addr struct with IP and port
// --------------------------------------------------------------------
#ifndef __linux__
typedef unsigned int in_addr_t;
#endif

bool TSocketClient::Initialize ( const char* pServer, int pPort  )
{
    int         x;
    PHOSTENT    phostent = NULL;


    // precaution
    if ( pServer == NULL || pPort <= 0 )
        return false;

    // try converting from dotted decimal form
    x = inet_addr ( pServer );

    if ( x != (in_addr_t)INADDR_NONE ) {                                       // success

        // put the addr in SOCKADDR struct
        vSockAddr.sin_addr.s_addr = x;
    }

    // check if local
    else if ( strcmp( pServer, "(local)" ) == 0 ) {

        x = inet_addr ( "127.0.0.1" );
        if ( x != (in_addr_t)INADDR_NONE ) {                                   // success

            // put the addr in SOCKADDR struct
            vSockAddr.sin_addr.s_addr = x;
        }
        else
            return false;
    }

    // try name resolution
    else {

        // use gethosbyname function and try resolving
        phostent = gethostbyname(pServer);
        if ( phostent  == NULL )
            return false;                                         // resolution failed

        // put the addr in SOCKADDR struct
        memcpy ( &(vSockAddr.sin_addr), phostent->h_addr, phostent->h_length );

    }

    // prepare the remaining sock addr struct
    vSockAddr.sin_family    =  AF_INET;                 // address family
    vSockAddr.sin_port      =  htons ( (uint16_t)pPort );         // port number

#ifdef __linux__
    g_flgLibReady = true;
#endif
    if (is_ssl)
    {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();        
    }
    return true;
}


// --------------------------------------------------------------------
// default constructor, does nothing besides a rest
// --------------------------------------------------------------------

TSocketClient::TSocketClient ( bool is_ssl_  ) : is_ssl(is_ssl_)
{
    vSocket     = INVALID_SOCKET;                       // handle

    memset ( &vSockAddr, 0, sizeof(vSockAddr));         // address struct

    memset ( vszErrMsg, 0, sizeof(vszErrMsg));          // internal error message store
}


// --------------------------------------------------------------------
// constructor, which parepares sock_addr for connection
// --------------------------------------------------------------------

TSocketClient::TSocketClient ( const char* pServer, int pPort, bool is_ssl_  ) : is_ssl(is_ssl_)
{
    vSocket     = INVALID_SOCKET;                       // handle

    memset ( &vSockAddr, 0, sizeof(vSockAddr));         // address struct

    memset ( vszErrMsg, 0, sizeof(vszErrMsg));          // internal error message store

    Initialize ( pServer, pPort );                      // initialize with specified values
}


// --------------------------------------------------------------------
// destructor       
// --------------------------------------------------------------------

TSocketClient::~TSocketClient  ()
{
    // will close socket if open and reset corres. member variables
    SockClose();
}
#ifdef __linux__


// --------------------------------------------------------------------
// Linux variant of SockInitLib
// --------------------------------------------------------------------

int TSocketClient::SockInitLib ( short pMajorVersion, short pMinorVersion ) {
    /** Not in use by the connector */

    // check if already initialized
    if ( TSocketClient::g_flgLibReady == 1 )
        return 1;                                           // successs

    int status;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    int socketfd ;


    memset(&host_info, 0, sizeof host_info);

    status = getaddrinfo("192.168.0.50", "2013", &host_info, &host_info_list);
    if(status!=0){
        return 1;
    }

    socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
        host_info_list->ai_protocol);
    if (socketfd == -1) {
        return 2;
    }
    // set the static memeber indicator
    TSocketClient::g_flgLibReady = 1;

    return 0;                   // success
}

// --------------------------------------------------------------------
// Linux variant of SockFinalizeLib
// --------------------------------------------------------------------

bool TSocketClient::SockFinalizeLib ( void ) {
    /** Not in use by the connector */

    // note
    // applications must make sure that all sockets are closed
    // since this is a static member function and does not
    // know about class instances

    // lib cleanup for winsock


    // set the static memeber indicator
    TSocketClient::g_flgLibReady = 0;

    return true;
}
#else
// --------------------------------------------------------------------
// STATIC, to initialize the windows socket library ( requirement for winsock )
// --------------------------------------------------------------------

int TSocketClient::SockInitLib ( short pMajorVersion, short pMinorVersion )
{
    short       v;
    int         iStatus;
    WSADATA     wd;

    // check if already initialized
    if ( TSocketClient::g_flgLibReady == 1 )
        return 1;                                           // successs

    // set the version required lo,hi
    v = MAKEWORD( pMinorVersion, pMajorVersion );

    // call the Winsock initialization function
    iStatus = WSAStartup ( v, &wd );
    if ( iStatus != 0 ) {

        // SetErrMsg ( true, "WSAStartup failed" );         // cannot call a non-static member function
        return -1;                                          // so failure indicated by return code
    }

    // check if the required version is available
    if ( LOBYTE( wd.wVersion ) != pMinorVersion ||
        HIBYTE( wd.wVersion ) != pMajorVersion ) {

            WSACleanup ();                                  // perform the cleanup

            // SetErrMsg ( false, "Version mismatch. Required: %d.%d, Available: %d.%d", pMajorVersion, pMinorVersion, HIBYTE( wd.wVersion ), LOBYTE( wd.wVersion ));
            return -2;
    }

    // set the static memeber indicator
    TSocketClient::g_flgLibReady = 1;

    return 0;                   // success
}

// --------------------------------------------------------------------
// to finalize the windows socket library
// --------------------------------------------------------------------

bool TSocketClient::SockFinalizeLib ( void )
{
    // note
    // applications must make sure that all sockets are closed
    // since this is a static member function and does not
    // know about class instances

    // lib cleanup for winsock
    WSACleanup ();

    // set the static memeber indicator
    TSocketClient::g_flgLibReady = 0;

    return true;
}


#endif
// --------------------------------------------------------------------
// to create a socket and connect to the server using specified sockaddr
// --------------------------------------------------------------------

bool TSocketClient::SockCreateAndConnect ( void ) {

    int         iStatus;
    SOCKET      s;

    // note
    // create and connect r merged due to nature of client application

    // PRECAUTION

    if (TSocketClient::g_flgLibReady == 0 || vSockAddr.sin_addr.s_addr == 0 ) {

        SetErrMsg ( false, "Winsock/SockAddr not initialized" );
        return false;
    }

    // CREATE

    // create a new stream socket
    s = socket (AF_INET, SOCK_STREAM, 0 );
    if ( s == INVALID_SOCKET ) {

        SetErrMsg ( true, "WSASocket failed" );
        return false;
    }

    // CONNECT

    // connect using the already prepared address
    iStatus = connect ( s, ( struct sockaddr* )&vSockAddr, sizeof(vSockAddr));
    if ( iStatus != 0 ) {

#ifdef __linux__

        close(s);//   windows clean-up
#else
        closesocket( s ); 
#endif
        SetErrMsg ( true, "WSAConnect failed - %d\n ",errno );
        return false;
    }

    // SUCCESS

    vSocket = s;
    
    if (is_ssl)
    {
        SSL_CTX * sslctx = SSL_CTX_new( SSLv23_client_method());
        SSL_CTX_set_options (sslctx, SSL_OP_SINGLE_DH_USE | SSL_OP_NO_SSLv2);

        //int use_cert = SSL_CTX_use_certificate_file(sslctx, "/serverCertificate.pem" , SSL_FILETYPE_PEM);
        //int use_prv = SSL_CTX_use_PrivateKey_file(sslctx, "/serverCertificate.pem", SSL_FILETYPE_PEM);

        ssl = SSL_new(sslctx);
        SSL_set_fd(ssl, (int)vSocket );
        
        if ( SSL_connect(ssl) == -1 ) 
        {
            is_ssl = false;
            //Error occurred, log and close down ssl
            ShutdownSSL();
            SetErrMsg ( true, "server doesn't work in ssl mode\n ");
            return false;
        }
    }

    return true;
} //TSocketClient::SockCreateAndConnect


// --------------------------------------------------------------------
// to write a chunk to the socket
// --------------------------------------------------------------------

bool TSocketClient::SockWriteChunk (const void* pBuffer, size_t pChunkSize, int& pBytesWritten )
{
    
    int     iStatus;

    // caller safe
    pBytesWritten = 0;

    // precaution -------- class level
    if ( !(TSocketClient::g_flgLibReady == 1 and vSocket != 0)) {

        SetErrMsg ( false, "Winsock/Class not initialized" );
        return false;
    }

    // precaution --------- function level
    if ( pBuffer == NULL || pChunkSize < 0 ) {

        SetErrMsg ( false, "SockWriteChunk, bad params" );      // zero bytes write is NOT an error
        return false;
    }

    // send
    iStatus = (int)sock_send ( (char*)pBuffer, pChunkSize);

    if ( iStatus == SOCKET_ERROR ) {

        int ret = is_ssl ? SSL_get_error(ssl, iStatus) : errno;
        SetErrMsg ( true, "WSASend failed: %d\n ", ret);
        return false;
    }

    // bytes sent
    pBytesWritten = iStatus;

    return true;
}

// --------------------------------------------------------------------
// to read a chunk from the socket
// --------------------------------------------------------------------

bool TSocketClient::SockReadChunk ( char* pBuffer, int& pBytesRead, int pChunkSize )
{
    int     iStatus;
    int     iTotal_read=0;
    // caller safe
    pBytesRead = 0;

    // precaution --------- class level
    if ( !(TSocketClient::g_flgLibReady == 1 and vSocket != 0)) {

        SetErrMsg ( false, "Winsock/Class not initialized" );
        return false;
    }

    // precaution --------- function level
    if ( pBuffer == NULL || pChunkSize < 0 ) {

        SetErrMsg ( false, "SockReadChunk, bad params" );           // zero bytes read is NOT an error
        return false;
    }

    // recv

    //char* buf= new char[100];

    while(true ){
         iStatus = (int)sock_recv ( &pBuffer[iTotal_read], pChunkSize-iTotal_read);


        if ( iStatus == SOCKET_ERROR ) {

            SetErrMsg ( true, "WSARecv failed\n " );
            return false;
        }
       iTotal_read=iTotal_read+iStatus;
       if(iTotal_read == pChunkSize)
         break;
       if(iStatus==0){
          if(iTotal_read != pChunkSize){
            
             return false; 
           }   
         else{
             
             break;
            }
    }
}
    // bytes recd
    pBytesRead = iTotal_read;

    return true;
}


// --------------------------------------------------------------------
// to close the current socket
// --------------------------------------------------------------------

void TSocketClient::ShutdownSSL() {

    SSL_shutdown(ssl);
    SSL_free(ssl);
}


void TSocketClient::SockClose ( void ) {

    int iStatus;

    // check if socket handle is valid
    if( vSocket != INVALID_SOCKET ) {

        // start a graceful shutdown
        shutdown ( vSocket, 0x02 );                 // SD_BOTH is equal to 0x02

        // use closesocket call
#ifdef __linux__
        iStatus = close( vSocket );
#else
        iStatus = closesocket( vSocket );
        if(WSAGetLastError () != WSANOTINITIALISED ){
            SetErrMsg ( true, "closesocket failed." );
        }else{
            vSocket = INVALID_SOCKET;
        }


#endif
        if( iStatus == SOCKET_ERROR)    // considered failed only if previously initialized
            SetErrMsg ( true, "closesocket failed." );
        else
            vSocket = INVALID_SOCKET;
        
        if (is_ssl)
            ShutdownSSL();
    }
}

// --------------------------------------------------------------------
// to set error message for the server class
// --------------------------------------------------------------------

bool TSocketClient::SetErrMsg ( bool flgIncludeWin32Error, const char* pszErrMsg, ... )
{
    // check if error message specified or clean-up required
    if( pszErrMsg ) {

        va_list args;

        va_start (args, pszErrMsg);

        vsprintf( vszErrMsg,pszErrMsg, args );
        va_end (args);

#ifndef __linux__
        if( flgIncludeWin32Error )
            sprintf_s(vszErrMsg + strlen(vszErrMsg), 256 - strlen(vszErrMsg), " WSA-Win32 status: %d", WSAGetLastError());
#endif
    }
    else
        vszErrMsg[0] = 0;


    return false;
}

