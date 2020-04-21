#include <stdio.h>
#include <string>
#include <tuple>
#include <memory>
#ifndef __linux__
#include <winsock.h>
#endif

#include "socket.hpp"

#include <errno.h>
#include <openssl/ssl.h>

//#include <openssl/err.h>
//#include <openssl/ossl_typ.h>

ssize_t TSocketClient::sock_recv (char *__buf, sock_buf_size __n)
{
    if (is_ssl)
        return SSL_read (ssl, __buf, __n);
    else
        return recv (vSocket, __buf, __n, 0);
}

ssize_t TSocketClient::sock_send(const char *__buf, sock_buf_size __n)
{
    if (is_ssl)
        return SSL_write (ssl, __buf, __n);
    else
        return send (vSocket, __buf, __n, 0);
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
// STATIC, to initialize the windows socket library ( requirement for winsock )
// --------------------------------------------------------------------

int TSocketClient::SockInitLib ( short pMajorVersion, short pMinorVersion )
{

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
// to finalize the windows socket library
// --------------------------------------------------------------------

bool TSocketClient::SockFinalizeLib ( void )
{
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

bool TSocketClient::SockCreateAndConnect ( void )
{
    int         iStatus;
    SOCKET      s;

    // note
    // create and connect r merged due to nature of client application

    // PRECAUTION

    if ( TSocketClient::g_flgLibReady == 0 || vSockAddr.sin_addr.s_addr == 0 ) {

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
}


// --------------------------------------------------------------------
// to create a socket and connect to the server using specified sockaddr
// --------------------------------------------------------------------
// send ip + port
std::tuple<int ,std::string,bool> TSocketClient::GetParamsFromPicker(void)
{
	int         iStatus;
	SOCKET      s;

	// note
	// create and connect r merged due to nature of client application

	// PRECAUTION

	if (TSocketClient::g_flgLibReady == 0 || vSockAddr.sin_addr.s_addr == 0) {

		SetErrMsg(false, "Winsock/SockAddr not initialized");
		return std::make_tuple(0, std::string(""), false);
	}

	// CREATE

	// create a new stream socket
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET) {

		SetErrMsg(true, "WSASocket failed");
		return std::make_tuple(0, std::string(" "), false);
	}

	// CONNECT
	
	// connect using the already prepared address
	std::unique_ptr <char[]> s_ip;
	char message_size[4] = { 0 };
	char s_port[4] = { 0 };
	int port;
	int m_size = 0;
	iStatus = connect(s, (struct sockaddr*)&vSockAddr, sizeof(vSockAddr));
	if (iStatus >= 0)
	{

		//get bffer size;
		recv(s, message_size, 4, 0);

		//m_size = atoi(message_size);
		m_size = *((int*)message_size) + 1;
		s_ip = std::unique_ptr<char[]>(new char[m_size]);
		memset(s_ip.get(), 0, m_size);
		recv(s, s_ip.get(), m_size - 1, 0);
		recv(s, s_port, 4, 0);
		port = *((int*)s_port);

	}
	
	
		
	

#ifdef __linux__

		close(s);//   windows clean-up
#else
		closesocket(s);
#endif
		vSocket = s;
		if (iStatus == -1)
		{
			SetErrMsg(true, "WSAConnect failed - %d\n ", errno);
			return std::make_tuple(0, std::string(""), false);
		}
	
		return std::make_tuple(port, std::string(s_ip.get()), true);

	// SUCCESS

	
	
	
}





// --------------------------------------------------------------------
// to write a chunk to the socket
// --------------------------------------------------------------------

bool TSocketClient::SockWriteChunk (const void* pBuffer, size_t pChunkSize, int& pBytesWritten )
{
    
    int     iStatus;

    // caller safe
    pBytesWritten = 0;

    // precaution -------- class level
    if ( !IsReady()) {

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
    if ( !IsReady()) {

        SetErrMsg ( false, "Winsock/Class not initialized" );
        return false;
    }

    // precaution --------- function level
    if ( pBuffer == NULL || pChunkSize < 0 ) {

        SetErrMsg ( false, "SockReadChunk, bad params" );           // zero bytes read is NOT an error
        return false;
    }

    // other initializations

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
// to read a complete signed message from a socket
// --------------------------------------------------------------------

bool TSocketClient::SockReadFullMsg ( char*& pBuffer, int& pMsgSize, int pDefChunkSize )
{
    bool   flgComplete;
    bool   flgError;
    bool   flgSignedMsg;          // message is a signed one
    bool   flgSignChecked;        // message start has been checked for a sign

    bool   iStatus; 

    int    iChunkSize;
    int    iCurrBytesRead, iTotalBytesRead;
    int    nBytesAvailable;

    char*  tmp; 
    char*  buf;                  // final buf, to be returned to caller

    // note
    // the function receives msgs in 2 modes. 1. Signed message, 2. Raw msg
    // Case signed message: it will read till it receives signature again
    // Case Raw msg: it will read till it can peek 
    // The signature is not a part of the block returned to caller
    // the raw message feature is not very reliable

    // initialization for caller parameters
    pBuffer     = NULL;
    pMsgSize    = 0;

    // local initializations
    tmp             = NULL;
    buf             = NULL;
    iTotalBytesRead = 0;
    nBytesAvailable = 0;

    flgComplete     = false;
    flgError        = false;
    flgSignedMsg    = false;
    flgSignChecked  = false;

    // determine the chunk size
    iChunkSize = ( pDefChunkSize <= 0 ) ? DEF_CHUNK_SIZE : pDefChunkSize;

    // loop to read from socket in chunks
    while( !flgComplete && !flgError ) {

        // increase buffer size to read next chunk
        tmp = buf;                                     // save existing buffer
        buf = new char[iTotalBytesRead+iChunkSize];

        // preserve the existing data and release the old buffer
        if( tmp ) {

            memcpy( buf, tmp, iTotalBytesRead );
            delete[] tmp;   
            tmp = NULL;
        }

        // read the chunk
        iStatus = SockReadChunk ( buf+iTotalBytesRead, iCurrBytesRead, iChunkSize );

        // check if IO failed
        if( iStatus != true ) {
            flgError = true;
            continue;
        }
        else {
            // printbuffer( "Read chunk", buf+iTotalBytesRead, iCurrBytesRead );            // ????? debug
            // printf ( "Read chunk: %d, Total: %d\n", iCurrBytesRead, iTotalBytesRead );   // ????? debug
            iTotalBytesRead += iCurrBytesRead;  // increase the total number of bytes read
        }

        // IO succeeded
        // 1. check for start signature if enough data
        // 2. check for end signature if enough data
        // 3. check for end thru peek if an un-signed message with enough data

        // check START signature only if sufficient data & not checked
        if( !flgSignChecked &&
            iTotalBytesRead >= MSG_SOCK_SIGN_SIZE ) {

                // first mark the message as checked for signature
                flgSignChecked = true;

                // check if start of msg is valid signature
                if( memcmp( buf, MSG_SOCK_SIGN, MSG_SOCK_SIGN_SIZE ) == 0 )
                    flgSignedMsg = true;             // mark the message as signed

                // if( flgSignedMsg )  printf( "\nMessage is signed\n\n" );  
                // else printbuffer( "Chunk", buf, iTotalBytesRead );
        }

        // check END with signature only if sufficient data & msg is signed
        if( flgSignedMsg == true && 
            iTotalBytesRead >= (MSG_SOCK_SIGN_SIZE*2)) {

                // check from the end of buffer
                if( memcmp ( buf + iTotalBytesRead - MSG_SOCK_SIGN_SIZE, MSG_SOCK_SIGN, MSG_SOCK_SIGN_SIZE ) == 0 )
                    flgComplete = true;        // full message has been recd.
        }

        // check END thru peek only if msg is known to be un-signed
        if( flgSignChecked == true && 
            flgSignedMsg == false ) {

#ifndef __linux__
                // check if something more is available on pipe
                flgError = ( ioctlsocket ( vSocket, FIONREAD, ( unsigned long* )&nBytesAvailable ) == SOCKET_ERROR ) ? true : false;
                if( flgError ) {
                    SetErrMsg ( true, "Ioctlsocket failed for peek" );
                    continue;
                }
#endif
                // check if read operation is complete
                // 1. read bytes were less than chunk size
                // 2. no more data is available in pipe
                flgComplete  = ( iCurrBytesRead < iChunkSize || nBytesAvailable == 0 ) ? true : false;
        }
    }


    // check if there has been error
    if( flgError ) {

        // release any allocated buffers
        if( buf ) {
            delete[] buf;
            buf = NULL;
        }

        return false;
    }

    // read completed successfully so adjust msg if signed
    if( flgSignedMsg == true ) {

        // remove start signature
        memmove( buf, buf + MSG_SOCK_SIGN_SIZE, iTotalBytesRead - MSG_SOCK_SIGN_SIZE );

        // reduce the number of bytes read by START & END signatures
        iTotalBytesRead -= ( MSG_SOCK_SIGN_SIZE*2 );

        // since we have extra bytes bcoz of end signature, why not zero terminate the buffer
        buf[iTotalBytesRead] = 0;
    }

    // transfer to caller, both buffer & number of bytes
    pBuffer     = buf;
    pMsgSize    = iTotalBytesRead;

    return true;
}


// --------------------------------------------------------------------
// to close the current socket
// --------------------------------------------------------------------

void TSocketClient::ShutdownSSL()
{
    SSL_shutdown(ssl);
    SSL_free(ssl);
}

void TSocketClient::SockClose ( void )
{
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


// --------------------------------------------------------------------
// to write the signature to socket to indicate start or end of msg
// --------------------------------------------------------------------

bool TSocketClient::WriteSignature ( void )
{
    int    x;

    return SockWriteChunk (MSG_SOCK_SIGN, MSG_SOCK_SIGN_SIZE, x  );
}

