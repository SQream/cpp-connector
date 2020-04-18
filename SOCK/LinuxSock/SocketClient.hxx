// ---------------------------------------------------------------------------------
//
// File:    SOCK_CLI.HXX
//
//
// Notes:   Socket client class inline functions
//
// ----------------------------------------------------------------------------------

#ifndef _SOCK_CLI_HXX
#define _SOCK_CLI_HXX


// --------------------------------------------------------------------
// to check if the class=>socket is properly initialized
// --------------------------------------------------------------------

bool TSocketClient::IsReady ( void )
{
    // check if listener socket is ready
    return ( TSocketClient::g_flgLibReady == 1 && vSocket != 0  ) ? true : false;
}


// --------------------------------------------------------------------
// get the last error that occured
// --------------------------------------------------------------------

inline const char* TSocketClient::GetErrMsg ( void )
{
    return vszErrMsg;
}


#endif
