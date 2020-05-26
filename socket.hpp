#include <cstdio>
#include <stdlib.h>
#include <string>
// #include <tuple>
#include <memory>

#include <errno.h>
#include <openssl/ssl.h>

#ifndef __linux__
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif

#ifndef __linux__
    #pragma comment(lib, "Ws2_32.lib")
    typedef int64_t sock_buf_size;
    typedef int64_t ssize_t;
#else
    typedef size_t sock_buf_size;
#endif

#ifndef _SOCK_CLI_HPP
#define _SOCK_CLI_HPP

#ifdef __linux__
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netdb.h>      // getaddrinfo()
    #include <unistd.h>     // close()
    
    #include <netinet/in.h>
    #include <stdarg.h>
    #include <sys/ioctl.h>

    #define PHOSTENT hostent*
    #define SOCKET unsigned int
    #define INVALID_SOCKET  (SOCKET)(~0)
    #define h_addr h_addr_list[0]
    #define SOCKET_ERROR            (-1)
#endif

// ---------------------------- develop print related -----------------
#define ping puts("ping");
#define puts(str) puts(str);
#define putss(str) puts(str.c_str());

// ----------------------- client socket class ------------------------
struct ssl_st;
typedef struct ssl_st SSL;

struct TSocketClient {

public:
        bool            SockCreateAndConnect    ( void );                                                                 // create and connect via socket    
        bool            SockWriteChunk          ( const void* pBuffer, size_t pChunkSize, int& pBytesWritten );              // write a chunk to socket
        bool            SockReadChunk           ( char* pBuffer, int& pBytesRead, int pChunkSize );                       // read a chunk from socket 
        void            SockClose               ( void );           // closes the existing open socket if any    

        TSocketClient   ( const char* pServer, int pPort , bool is_ssl_ );   // constructor
        ~TSocketClient  ();                                                  // destructor       

private:
        // data members
        SOCKET          vSocket;                        // socket handle
        struct sockaddr_in  vSockAddr;                  // sockaddr details
        SSL *ssl;
        bool is_ssl;
        // other data members
        char vszErrMsg[256];                     // last error msg

        // private function members
        bool    SetErrMsg               ( bool flgIncludeWin32Error, const char* pszErrMsg, ... );
        ssize_t sock_send(const char *buf, sock_buf_size buf_size);
        ssize_t sock_recv(char *buf, sock_buf_size buf_size);
        
public:
        // static data members
        static  bool        g_flgLibReady;        

        // static member functions
        static int  SockInitLib     (short pMajorVersion = 1, short pMinorVersion = 1);
        static bool SockFinalizeLib (void);
};

#endif   // of hpp


ssize_t TSocketClient::sock_recv (char *buf, sock_buf_size buf_size) {

    return (is_ssl) ? SSL_read(ssl, buf, buf_size) : recv(vSocket, buf, buf_size, 0);
}

ssize_t TSocketClient::sock_send(const char *buf, sock_buf_size buf_size) {

        return (is_ssl) ? SSL_write(ssl, buf, buf_size) : send(vSocket, buf, buf_size, 0);
}

// ----------------- static data members initialization ------------------
bool TSocketClient::g_flgLibReady = 0;

// --------------------------------------------------------------------
// does the work of the constructor, initializes addr struct with IP and port
// --------------------------------------------------------------------
#ifndef __linux__
typedef unsigned int in_addr_t;
#endif

// --------------------------------------------------------------------
// constructor, which parepares sock_addr for connection
// --------------------------------------------------------------------

TSocketClient::TSocketClient (const char* pServer, int pPort, bool is_ssl_) : is_ssl(is_ssl_) {

    vSocket = INVALID_SOCKET;                          // handle
    memset (&vSockAddr, 0, sizeof(vSockAddr));         // address struct
    memset (vszErrMsg, 0, sizeof(vszErrMsg));          // internal error message store
    auto error = false;
    // Initialize 
    int addr;
    PHOSTENT phostent = NULL;
    struct addrinfo *result = NULL;
    
    // precaution
    if ( pServer == NULL or pPort <= 0 )
        error = true;

    // try converting from dotted decimal form
    inet_pton(AF_INET, pServer, &addr);
    // addr = inet_addr (pServer);
    if (addr != (in_addr_t)INADDR_NONE) {                                       // success
        // put the addr in SOCKADDR struct
        vSockAddr.sin_addr.s_addr = addr;
    }

    // check if local
    else if (strcmp(pServer, "(local)") == 0) {
        inet_pton(AF_INET, "127.0.0.1", &addr);
        // addr = inet_addr ("127.0.0.1");
        if (addr != (in_addr_t)INADDR_NONE) {                                   // success
            // put the addr in SOCKADDR struct
            vSockAddr.sin_addr.s_addr = addr;
        }
        else
            error = true;
    }
 
    // try name resolution
    else {
        // use gethosbyname function and try resolving
        getaddrinfo(pServer, NULL, {}, &result);
        // phostent = gethostbyname(pServer);
        if ( phostent  == NULL )
            error  = true;                                         // resolution failed

        // put the addr in SOCKADDR struct
        memcpy (&(vSockAddr.sin_addr), phostent->h_addr, phostent->h_length);
    }

    // prepare the remaining sock addr struct
    vSockAddr.sin_family    =  AF_INET;                 // address family
    vSockAddr.sin_port      =  htons ( (uint16_t)pPort );         // port number

#ifdef __linux__
    g_flgLibReady = true;
#endif
    if (is_ssl) {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();        
    }

}


TSocketClient::~TSocketClient  () {

    // will close socket if open and reset corres. member variables
    SockClose();
}

// --------------------------------------------------------------------
// STATIC, to initialize the windows socket library ( requirement for winsock )
// --------------------------------------------------------------------
#ifndef __linux__  // SockInitLib, SockFinalizeLib - Winsock init / end

int TSocketClient::SockInitLib (short pMajorVersion, short pMinorVersion) {

    short    v;
    int      err;
    WSADATA  wd;

    // check if already initialized
    if ( TSocketClient::g_flgLibReady == 1 )
        return 1;                                           

    // call the Winsock initialization function
    // https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsastartup
    if (WSAStartup (MAKEWORD(pMinorVersion, pMajorVersion), &wd)) {
        // SetErrMsg ( true, "WSAStartup failed" );         // cannot call a non-static member function
        return -1;                                          // so failure indicated by return code
    }

    // Confirm Winsock.dll version
    if (LOBYTE(wd.wVersion) != pMinorVersion or HIBYTE(wd.wVersion) != pMajorVersion) {
        WSACleanup ();                                
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

#endif  // SockInitLib, SockFinalizeLib

bool TSocketClient::SockCreateAndConnect ( void ) {

    int    iStatus;
    SOCKET s;

    // PRECAUTION
    if (TSocketClient::g_flgLibReady == 0 || vSockAddr.sin_addr.s_addr == 0 ) {
        SetErrMsg ( false, "Winsock/SockAddr not initialized" );
        puts ("*** inside precaution\n")
        return false;
    }

    // CREATE - create a new stream socket
    s = socket (AF_INET, SOCK_STREAM, 0 );
    if ( s == INVALID_SOCKET ) {
        SetErrMsg ( true, "WSASocket failed" );
        return false;
    }

    // CONNECT -  connect using the already prepared address
    iStatus = connect ( s, ( struct sockaddr* )&vSockAddr, sizeof(vSockAddr));
    if ( iStatus != 0 ) {
        puts ("*** inside connect fail\n")
        printf ("%d", iStatus);
        #ifdef __linux__
        close(s);
        #else
        closesocket(s); 
        #endif
        SetErrMsg ( true, "WSAConnect failed - %d\n ",errno );
        return false;
    }

    // SUCCESS
    vSocket = s;
    
    // SSL
    if (is_ssl) {
        SSL_CTX * sslctx = SSL_CTX_new( SSLv23_client_method());
        SSL_CTX_set_options (sslctx, SSL_OP_SINGLE_DH_USE | SSL_OP_NO_SSLv2);
        ssl = SSL_new(sslctx);
        SSL_set_fd(ssl, (int)vSocket );
        
        if (SSL_connect(ssl) == -1) {
            is_ssl = false;
            SSL_shutdown(ssl);
            SSL_free(ssl);
            SetErrMsg ( true, "server doesn't work in ssl mode\n ");
            return false;
        }
    } 

    return true;
} //TSocketClient::SockCreateAndConnect


// --------------------------------------------------------------------
// to write a chunk to the socket
// --------------------------------------------------------------------

bool TSocketClient::SockWriteChunk (const void* pBuffer, size_t pChunkSize, int& pBytesWritten ) {
    
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

bool TSocketClient::SockReadChunk (char* pBuffer, int& pBytesRead, int pChunkSize) {

    int  bytes_read;
    int  total_read = 0;
    // caller safe
    pBytesRead = 0;

    // precaution --------- class level
    if (!(TSocketClient::g_flgLibReady == 1 and vSocket != 0)) {
        SetErrMsg ( false, "Winsock/Class not initialized" );
        return false;
    }

    // precaution --------- function level
    if ( pBuffer == NULL || pChunkSize < 0 ) {
        SetErrMsg ( false, "SockReadChunk, bad params" );           // zero bytes read is NOT an error
        return false;
    }

    // recv
    while(true) {
        bytes_read = (int)sock_recv (&pBuffer[total_read], pChunkSize - total_read);
        if ( bytes_read == SOCKET_ERROR ) {  // -1 bytes read means error
            SetErrMsg (true, "WSARecv failed\n ");
            return false;
        }
        total_read = total_read + bytes_read;
        if(total_read == pChunkSize)
            break;
        if(bytes_read == 0) {
            if(total_read != pChunkSize)
                return false; 
            else 
                break;
        }
    }

    // bytes recd
    pBytesRead = total_read;

    return true;
}


void TSocketClient::SockClose (void) {

    int iStatus;

    // check if socket handle is valid
    if(vSocket != INVALID_SOCKET) {
        // start a graceful shutdown
        shutdown (vSocket, 0x02);                 // SD_BOTH is equal to 0x02

        // use closesocket call
#ifdef __linux__
        iStatus = close(vSocket);
#else
        iStatus = closesocket(vSocket);
        if(WSAGetLastError () != WSANOTINITIALISED)
            SetErrMsg (true, "closesocket failed.");
        else
            vSocket = INVALID_SOCKET;
#endif
        if(iStatus == SOCKET_ERROR)    // considered failed only if previously initialized
            SetErrMsg ( true, "closesocket failed." );
        else
            vSocket = INVALID_SOCKET;
        
        if (is_ssl) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
        }
    }
}


bool TSocketClient::SetErrMsg (bool flgIncludeWin32Error, const char* pszErrMsg, ...) {

    // check if error message specified or clean-up required
    if(pszErrMsg) {
        va_list args;
        va_start (args, pszErrMsg);
        vsprintf_s( vszErrMsg,pszErrMsg, args );
        va_end (args);

#ifndef __linux__
        if(flgIncludeWin32Error)
            sprintf_s(vszErrMsg + strlen(vszErrMsg), 256 - strlen(vszErrMsg), " WSA-Win32 status: %d", WSAGetLastError());
#endif
    }
    else
        vszErrMsg[0] = 0;

    return false;
}
