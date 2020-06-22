/* SQream C++ Connector
*  Originally by - Benny Van-Zuiden, Jeremy Chetboul, 2018
*/ 

#include "connector.h"
#include "socket.hpp"
#include "json.hpp"
#include <exception>

/// Macro to format and throw errors
#define THROW_GENERAL_ERROR(MSG) throw std::string(__FILE__":")+std::to_string(__LINE__)+std::string(" in ")+std::string(__func__)+std::string("(): ")+std::string(MSG)
#define THROW_SQREAM_ERROR(MSG) throw std::string(__FILE__":")+std::to_string(__LINE__)+std::string(" in ")+std::string(__func__)+std::string("() returned error from SQream: ")+std::string(MSG)

// Easy prints for debugging
#define ping puts("ping");
#define puts(str) puts(str);
#define putss(str) puts(str.c_str());
#define putj(jsn) puts(jsn.dump().c_str());

///Linux-Windows snprintf variant
#ifdef __linux__
    #define SNPRINTF snprintf
#else
    #define SNPRINTF _snprintf
#endif

using json = nlohmann::json;

template<typename ...Args> void sqream::MESSAGES::format(std::vector<char> &output,const char input[],Args...args) {
    /// <i>sqream connector JSON message formatter</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li><tt>std::vector &output</tt>:&emsp; output buffer</li>
    /// <li><tt>const charf input[]</tt>:&emsp; input cstring</li>
    /// <li><tt>Args...args</tt>:&emsp; variadic input for MESSAGE arguments</li>
    /// </ul>
    /// This function is basically a snprintf wrapper.
    const size_t I=1+SNPRINTF(nullptr,0,input,args...);
    output.resize(I);
    SNPRINTF(output.data(),I,input,args...);
    output.pop_back();
}


/// <i>ERR_HANDLE macro removes redundant code to check the validity of a server response</i><br>
#define ERR_HANDLE(VALUE,JSON_TYPE)\
{\
    if(reply_json.contains(#VALUE)) return reply_json[#VALUE];\
    else if(reply_json.contains("error")) THROW_SQREAM_ERROR(reply_json["error"]);\
    else THROW_GENERAL_ERROR("an unknown error occured");\
}

/// <i>ERR_HANDLE_STR macro removes redundant code to check the validity of a server response in case the expected response is cstring</i><br>
#define ERR_HANDLE_STR(VALUE)\
{\
    if(reply_json.contains(#VALUE)) return !(reply_json[#VALUE] == #VALUE);\
    else if(reply_json.contains("error")) THROW_SQREAM_ERROR(reply_json["error"]);\
    else THROW_GENERAL_ERROR("an unknown error occured");\
}

bool verify_response(json& reply_json, std::string value) {

    if(reply_json.contains(value)) 
        return (reply_json[value] == value);
    else if(reply_json.contains("error")) { 
        THROW_SQREAM_ERROR(reply_json["error"]);
    }else {
        puts("verify else clause");
        putss(value)
        puts(reply_json.dump().c_str());
        THROW_GENERAL_ERROR("an unknown error occured");
    }
}


static void rxtx(sqream::connector *conn, json& reply_json,const char input[]) ///< <h3>Method to send and receive formatted messages</h3>
{
    /// <i>Routine to perform a send and receive of formatted JSON messages</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>sqream::connector *conn:&emsp; Pointer to SQream low level connector type</li>
    /// <li>json &reply_json:&emsp; JSON reply message from sqreamd</li>
    /// <li>const char input[]:&emsp; JSON message to sqreamd</li>
    /// </ul>

    std::vector<char> reply_msg;
    
    conn->write(input,strlen(input),sqream::HEADER::HEADER_JSON);
    conn->read(reply_msg);
    
    // add catch error - https://github.com/nlohmann/json/blob/develop/doc/examples/parse_error.cpp
    reply_json = json::parse(std::string(reply_msg.begin(),reply_msg.end()).c_str()); // THROW_GENERAL_ERROR("could not parse server response");
}

template<typename ...Args> 
void rxtx(sqream::connector *conn, json& reply_json,const char input[],Args...args) ///< <h3>Method to send and receive unformatted messages</h3>
{
    /// <i>Routine to perform a send and receive of unformatted JSON messages</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>sqream::connector *conn:&emsp; Pointer to SQream low level connector type</li>
    /// <li>json &reply_json:&emsp; JSON reply message from sqreamd</li>
    /// <li>const char input[]:&emsp; JSON message to sqreamd</li>
    /// <li>Args..args:&emsp; variadic argument to format the unformatted JSON messages</li>
    /// </ul>
    std::vector<char> reply_msg,msg;
    sqream::MESSAGES::format(msg,input,args...);
    conn->write(msg.data(),msg.size(),sqream::HEADER::HEADER_JSON);
    conn->read(reply_msg);
    
    // add catch error - https://github.com/nlohmann/json/blob/develop/doc/examples/parse_error.cpp
    reply_json = json::parse(std::string(reply_msg.begin(),reply_msg.end()).c_str()); //THROW_GENERAL_ERROR("could not parse server response");

}


//         --- Connector object ----
//         -------------------------

sqream::connector::connector() {
    /// <i>Trivial connector constructor</i><br>

    /// <i>ensure the socket is null pointer on object creation</i><br>
    socket=nullptr;
}

sqream::connector::~connector() {
   
    /// <i>Connector destructor that automatically disconnects</i><br>
    if(socket) {

        int bytes_written;
        const size_t data_size=strlen(MESSAGES::closeConnection);
        const size_t block_size=HEADER::SIZE+sizeof(data_size);
        std::vector<char> pillow(block_size+data_size);
        memcpy(pillow.data(),HEADER::HEADER_JSON,HEADER::SIZE);
        memcpy(&pillow[HEADER::SIZE],&data_size,sizeof(data_size));
        memcpy(&pillow[block_size],MESSAGES::closeConnection,data_size);
        socket->SockWriteChunk(pillow.data(),data_size+block_size,bytes_written);
    
        /// <i>ensure a disconnect on object destruction</i><br>
        /// <i>disconnect from a sqreamd session</i><br>
        socket->SockClose();
        delete socket;
        socket=nullptr;
    }
}


void sqream::connector::connect_socket(const std::string &ipv4,int port,bool ssl) {

    /// <i>connect to a sqreamd session</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>const std::string &ipv4:&emsp; ipv4 address of the sqreamd</li>
    /// <li>int port:&emsp; port of the sqreamd</li>
    /// </ul>

    if(socket) {
        socket->SockClose();
        delete socket;
        socket=nullptr;
    }
    socket=new(std::nothrow) TSocketClient(ipv4.c_str(),port,ssl);

    if(socket->SockCreateAndConnect()==false) {
        socket=nullptr;
        THROW_GENERAL_ERROR("unable to create socket");
    }
}


void sqream::connector::read(std::vector<char> &data) {

    /// <i>read data sent by sqreamd</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>std::vector<char> &data:&emsp; output buffer (automatically resized)</li>
    /// </ul>
    if(socket) {
        char header[10];
        uint64_t data_size;
        int bytes_read;
        if(!socket->SockReadChunk(header,bytes_read,sizeof(header))) THROW_GENERAL_ERROR("socket failed to read header");
        if(header[0]!=HEADER::PROTOCOL_VERSION) THROW_GENERAL_ERROR("protocol version mismatch");
        memcpy(&data_size,&header[2],sizeof(uint64_t));
        data.resize(data_size);
        if(!socket->SockReadChunk((char*)data.data(),bytes_read,data_size)) THROW_GENERAL_ERROR("socket failed to read content");
    }
    else 
        THROW_GENERAL_ERROR("not connected");
}


void sqream::connector::write(const char *data,const uint64_t data_size,const uint8_t msg_type[HEADER::SIZE]) {

    /// <i>read data sent by sqreamd</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>const char *data:&emsp; pointer to data to be written</li>
    /// <li>const size_t data_size:&emsp; size of data to be written</li>
    /// <li>const uint8_t msg_type[HEADER::SIZE]: message type indicator</li>
    /// </ul>
    if(socket) {
        if(data_size<CONSTS::MAX_SIZE) {
            int bytes_written;
            if(!memcmp((const char*)msg_type,HEADER::HEADER_JSON,HEADER::SIZE)) {
                const size_t block_size=HEADER::SIZE+sizeof(data_size);
                std::vector<char> pillow(block_size+data_size);
                memcpy(pillow.data(),msg_type,HEADER::SIZE);
                memcpy(&pillow[HEADER::SIZE],&data_size,sizeof(data_size));
                memcpy(&pillow[block_size],data,data_size);
                if(!socket->SockWriteChunk(pillow.data(),data_size+block_size,bytes_written)) THROW_GENERAL_ERROR("socket failed to write message block");
            }
            else {
                if(!socket->SockWriteChunk(msg_type,HEADER::SIZE,bytes_written)) THROW_GENERAL_ERROR("socket failed to write header");
                if(!socket->SockWriteChunk(&data_size,sizeof(data_size),bytes_written)) THROW_GENERAL_ERROR("socket failed to write binary data size");
                if(!socket->SockWriteChunk(data,data_size,bytes_written)) THROW_GENERAL_ERROR("socket failed to write binary data");
            }
        }
        else THROW_GENERAL_ERROR("binary data overflow");
    }
    else THROW_GENERAL_ERROR("not connected");
}


bool sqream::connector::connect(const std::string &ipv4,int port,bool ssl,const std::string &username,const std::string &password,const std::string &database,const std::string &service) {
    /// <i>Connector routine that connects to a given ipv4, port, database on sqreamd</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>const std::string &ipv4:&emsp; ipv4 of sqreamd</li>
    /// <li>int port:&emsp; port of sqreamd</li>
    /// <li>bool ssl:&emsp; connect with an SSL session</li>
    /// <li>const std::string &username:&emsp; username</li>
    /// <li>const std::string &password:&emsp; password</li>
    /// <li>const std::string &database:&emsp; database name</li>
    /// </ul>
    /// <b>return</b>(uint32_t):&emsp; connection_id
    connect_socket(ipv4,port,ssl);

    json reply_json;
    rxtx(this, reply_json, MESSAGES::connectDatabase, service.c_str(), username.c_str(), password.c_str(), database.c_str());
    ipv4_=ipv4;
    port_=port;
    ssl_=ssl;
    username_=username;
    password_=password;
    database_=database;
    service_=service;
    var_encoding_= "ascii";
    if(reply_json.contains("varcharEncoding")) 
        var_encoding_ = reply_json["varcharEncoding"];          // std::string var_encoding_

    if(reply_json.contains("connectionId")) {
        connection_id_ = reply_json["connectionId"];  // uint32_t connection_id_
        return true;
    }
    else 
        return false;
}

bool sqream::connector::reconnect(const std::string &ipv4,int port,int listener_id) {

    connect_socket(ipv4,port,ssl_);
    json reply_json;
    rxtx(this, reply_json,MESSAGES::reconnectDatabase,database_.c_str(),service_.c_str(),connection_id_,username_.c_str(),password_.c_str(),listener_id);
    
    return reply_json.contains("databaseConnected");
}


bool sqream::connector::open_statement() {
    /// <i>Connector routine that opens a new statement on sqreamd</i><br>
    /// <b>return</b>(uint32_t):&emsp; statement_id
    json reply_json;
    rxtx(this, reply_json,MESSAGES::getStatementId);
    if(reply_json.contains("statementId")) {
        statement_id_=reply_json["statementId"];
        return true;
    }
    else 
        return false;
}

bool sqream::connector::prepare_statement(std::string sqlQuery,int chunk_size) {
    /// <i>Connector routine that prepares a statement on sqreamd</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>std::string sqlQuery:&emsp; sql query string (this is the actual sql query)</li>
    /// <li>int chunk_size:&emsp; this parameter is unparsed</li>
    /// </ul>
    /// <b>return</b>(bool):&emsp; success from server
    json reply_json, prepare_json;
    prepare_json["prepareStatement"] = sqlQuery;
    prepare_json["chunkSize"] = chunk_size;
	rxtx(this, reply_json, prepare_json.dump().c_str());

    if(reply_json.contains("reconnect") and (reply_json["reconnect"] == true)) {     
        if((reply_json.contains("port") or reply_json.contains("port_ssl")) and reply_json.contains("ip") and reply_json.contains("listener_id")) {
            const int port = ssl_ ? reply_json["port_ssl"] : reply_json["port"];
            if(reconnect(reply_json["ip"], port, reply_json["listener_id"]));
            else THROW_GENERAL_ERROR("reconnection failed");
        }
        else THROW_GENERAL_ERROR("could not parse reconnection message");
        rxtx(this, reply_json, MESSAGES::reconstructStatement,statement_id_);
        // ERR_HANDLE_STR(statementReconstructed)
        return verify_response(reply_json, "statementReconstructed");
    }
    else { 
        ERR_HANDLE(statementPrepared,GetBool)
    }
}

sqream::CONSTS::statement_type sqream::connector::metadata_query(std::vector<column> &columns_metadata_in,std::vector<column> &columns_metadata_out)
{
    /// <i>Connector routine that retrieves the metadata the statement is going to read/write</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>std::vector<column> & columns_metadata_in:&emsp; metadata container on input table (for network instert)</li>
    /// <li>std::vector<column> & columns_metadata_out:&emsp; metadata container on output table (for any select)</li>
    /// </ul>
    /// <b>return</b>(sqream::CONSTS::statement type): statement type
    /// statement type can be either unset,select,insert or direct:
    /// unset is returned when a statement fails
    /// select is returned when a statement requires an output buffer
    /// insert is returned when a statement requires an input buffer
    /// direct is returned when a statement does not requires input nor output
    columns_metadata_out.clear();
    columns_metadata_in.clear();
    CONSTS::statement_type retval=CONSTS::statement_type::unset;
    json queryTypeOut_reply_json;
    rxtx(this, queryTypeOut_reply_json,MESSAGES::queryTypeOut);
    if(queryTypeOut_reply_json.contains("queryTypeNamed") and queryTypeOut_reply_json["queryTypeNamed"].is_array() and queryTypeOut_reply_json["queryTypeNamed"].size())
    {
        retval=CONSTS::statement_type::select;
        columns_metadata_out.resize(queryTypeOut_reply_json["queryTypeNamed"].size());
        const json &out_array = queryTypeOut_reply_json["queryTypeNamed"];
        auto out_size = out_array.size();
        for(json::size_type i=0; i<out_size ;i++)
        {
            uint8_t checksum=0;
            if(out_array[i].contains("isTrueVarChar")) columns_metadata_out[i].is_true_varchar = out_array[i]["isTrueVarChar"], checksum|=1;
            if(out_array[i].contains("nullable")) columns_metadata_out[i].nullable = out_array[i]["nullable"], checksum|=2;
            if(out_array[i].contains("name")) columns_metadata_out[i].name=std::string(out_array[i]["name"]), checksum|=4;
            if(out_array[i].contains("type") and out_array[i]["type"].is_array() and out_array[i]["type"].size()==3)
            {
                columns_metadata_out[i].type=std::string(out_array[i]["type"][0]);
                columns_metadata_out[i].size=out_array[i]["type"][1];
                columns_metadata_out[i].scale=out_array[i]["type"][2];
                checksum|=8;
            }
            if(checksum!=15) THROW_GENERAL_ERROR("could not parse metadata out");
        }
    }
    else
    {
        json queryTypeIn_reply_json;
        rxtx(this, queryTypeIn_reply_json,MESSAGES::queryTypeIn);
        if(queryTypeIn_reply_json.contains("queryType") and queryTypeIn_reply_json["queryType"].is_array() and queryTypeIn_reply_json["queryType"].size())
        {
            retval=CONSTS::statement_type::insert;
            columns_metadata_in.resize(queryTypeIn_reply_json["queryType"].size());
            const json &in_array = queryTypeIn_reply_json["queryType"];
            for(size_t i=0; i<in_array.size(); i++)
            {
                uint8_t checksum=0;
                if(in_array[i].contains("isTrueVarChar")) columns_metadata_in[i].is_true_varchar = in_array[i]["isTrueVarChar"], checksum|=1;
                if(in_array[i].contains("nullable")) columns_metadata_in[i].nullable = in_array[i]["nullable"], checksum|=2;
                if(in_array[i].contains("type") and in_array[i]["type"].is_array() and in_array[i]["type"].size()==3)
                {
                    columns_metadata_in[i].type=std::string(in_array[i]["type"][0]);
                    columns_metadata_in[i].size=in_array[i]["type"][1];
                    columns_metadata_in[i].scale=in_array[i]["type"][2];
                    checksum|=4;
                }
                if(checksum!=7) THROW_GENERAL_ERROR("could not parse metadata in");
            }
        }
        else retval=CONSTS::statement_type::direct;
    }
    return retval;
}

bool sqream::connector::execute()
{
    /// <i>Connector routine that tells the server to execute a statement</i><br>
    /// <b>return</b>(bool):&emsp; success response from sqreamd
    json reply_json;
    rxtx(this, reply_json,MESSAGES::execute);

    // ERR_HANDLE_STR(executed)
    bool res = verify_response(reply_json, "executed");
    return res;
    // puts ("execute end");

}

size_t sqream::connector::fetch(std::vector<char> &binary_data,std::vector<uint64_t> &column_sizes,size_t min_size)
{
    /// <i>Connector routine that retrieves serialized output data from the server</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>std::vector<char> &binary_data:&emsp; retrieved data buffer</li>
    /// <li>size_t min_size=1:&emsp; keep retrieving until at least size of bytes is retrieved (default value is 1)</li>
    /// </ul>
    /// <b>return</b>(size_t):&emsp; number of rows
    json reply_json;
    binary_data.resize(0);
    column_sizes.resize(0);
    size_t row_count=0;
    while(binary_data.size()<min_size)
    {
        rxtx(this, reply_json,MESSAGES::fetch);
        if(reply_json.contains("colSzs") and reply_json.contains("rows"))
        {
            if(reply_json["colSzs"].is_array() and reply_json["colSzs"].size())
            {
                row_count += int(reply_json["rows"]);
                const json &array = reply_json["colSzs"];
                const auto I = array.size();
                if(column_sizes.size() != I) column_sizes.resize(I);
                size_t binary_size=0;
                for(json::size_type i=0; i<I ;i++) {
                    binary_size += column_sizes[i] = array[i];
                }
                if(binary_size>0)
                {       
                    std::vector<char> temp(binary_size);
                    read(temp);
                    binary_data.insert(binary_data.end(),temp.begin(),temp.end());
                }
                else break;
            }
            else break;
        }
        else if(reply_json.contains("error")) THROW_SQREAM_ERROR(reply_json["error"]);
        else THROW_GENERAL_ERROR("sqream::connector::fetch: an unknown error occured");
    }
    return row_count;
}

void sqream::connector::put(std::vector<char> &binary_data,size_t rows)
{
    /// <i>Connector routine that sends serialized input data to the server</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>std::vector<char> &binary_data:&emsp; input data buffer</li>
    /// <li>size_t rows:&emsp; number of rows that the input data buffer contains</li>
    /// </ul>
    std::vector<char> msg,reply_msg;
    json reply_json;
    MESSAGES::format(msg,MESSAGES::put,rows);
    write(msg.data(),msg.size(),HEADER::HEADER_JSON);
    write(binary_data.data(),binary_data.size(),HEADER::HEADER_BINARY);
    read(reply_msg);
    reply_json = json::parse(std::string(reply_msg.begin(),reply_msg.end()).c_str());
    if(reply_json.contains("putted") and (reply_json["putted"] == "putted")) 
        return;
    else if(reply_json.contains("error")) 
        THROW_SQREAM_ERROR(reply_json["error"]);
    else 
        THROW_GENERAL_ERROR("sqream::connector::put: an unknown error occured");
}

bool sqream::connector::close_statement()
{
    /// <i>Connector routine that closes a statement indicating it will not be used again</i><br>
    /// <b>return</b>(bool):&emsp; success response from sqreamd
    json reply_json;
    rxtx(this, reply_json,MESSAGES::closeStatement);
    // ERR_HANDLE_STR(statementClosed)
    return verify_response(reply_json, "statementClosed");
}

#undef ERR_HANDLE
#undef ERR_HANDLE_STR


//   ----  Driver object
//   -------------------

sqream::driver::driver() {

    /// <i>Trivial connector constructor</i><br>
    statement_type_=CONSTS::unset;
    sqc_=nullptr;
    buffer_switch_th.reset(nullptr);
    buffer_.reserve(CONSTS::MIN_PUT_SIZE);
//*
#ifndef __linux__
    sqc_->socket->SockInitLib();
#endif
//*/
}

sqream::driver::~driver()
{
    /// <i>Destructor that closes a statement if available and disconnects from sqreamd</i><br>
    if(buffer_switch_th) {
        //std::printf("Ending previous buff switch\n");
        (*buffer_switch_th).get();
        buffer_switch_th.reset(nullptr);
    }
    if(state_>0 and state_<7 and sqc_->socket) {
        int bytes_read_write;
        const size_t data_size=strlen(MESSAGES::closeStatement);
        const size_t block_size=HEADER::SIZE+sizeof(data_size);
        std::vector<char> pillow(block_size+data_size);
        memcpy(pillow.data(),HEADER::HEADER_JSON,HEADER::SIZE);
        memcpy(&pillow[HEADER::SIZE],&data_size,sizeof(data_size));
        memcpy(&pillow[block_size],MESSAGES::closeStatement,data_size);
        sqc_->socket->SockWriteChunk(pillow.data(),data_size+block_size,bytes_read_write);
        char header[10];
        uint64_t data_size_out;
        sqc_->socket->SockReadChunk(header,bytes_read_write,sizeof(header));
        memcpy(&data_size_out,&header[2],sizeof(uint64_t));
        std::vector<char> data(data_size);
        sqc_->socket->SockReadChunk((char*)data.data(),bytes_read_write,data_size);
    }
    disconnect();
#ifndef __linux__
    sqc_->socket->SockFinalizeLib();
#endif
}

bool sqream::driver::connect(const std::string &ipv4,int port,bool ssl,const std::string &username,const std::string &password,const std::string &database,const std::string &service) {
    /// <i>Connector routine that connects to a given ipv4, port, database on sqreamd</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>const std::string &ipv4:&emsp; ipv4 of sqreamd</li>
    /// <li>int port:&emsp; port of sqreamd</li>
    /// <li>bool ssl:&emsp; connect with an SSL session</li>
    /// <li>const std::string &username:&emsp; username</li>
    /// <li>const std::string &password:&emsp; password</li>
    /// <li>const std::string &database:&emsp; database name</li>
    /// </ul>
    /// <b>return</b>(bool):&emsp; successful
    if(sqc_) 
        disconnect();
    sqc_=new(std::nothrow) connector;
    if(!sqc_) 
        THROW_GENERAL_ERROR("error creating connection");

    return sqc_->connect(ipv4,port,ssl,username,password,database,service);
}

void sqream::driver::disconnect() {
    /// <i>Disconnect from sqreamd</i><br>
    if(sqc_) {
        delete sqc_;
        sqc_=nullptr;
    }
}

/// Standard error: thrown if socket does not exist
#define TC(X) if(!X) THROW_GENERAL_ERROR("sqream driver is not connected");
/// Standard error: thrown if function is called when it is not supposed to yet
#define CS(X) if(state_!=X) THROW_GENERAL_ERROR("protocol order violation");
/// Standard error: thrown if a non existing column is set
#define CI(X) if(X>=metadata_input_.size()) THROW_GENERAL_ERROR("input column does not exist in query");
/// Standard error: thrown if a non existing column is getten
#define CO(X) if(X>=metadata_output_.size()) THROW_GENERAL_ERROR("output column does not exist query");
/// Combinator of TC and CS macros
#define TCCS(X,Y) TC(X) CS(Y)
/// Combinator of TC, CS and CO macros
#define TCCSCO(X,Y,Z) TC(X) CS(Y) CO(Z)
/// Combinator of TC, CS and CI macros
#define TCCSCI(X,Y,Z) TC(X) CS(Y) CI(Z)

size_t sqream::driver::flat_size_() {
    /// <i>Size of the flat buffer when a unflat pbuffer is converted to it</i><br>
    /// <b>return</b>(size_t):&emsp; size of flat buffer
    size_t retval=0;
    for(std::vector<std::vector<char>> &cols:pbuffer_[curr_buff_idx]) for(std::vector<char> &col:cols) retval+=col.size();
    return retval;
}

void sqream::driver::init_pbuffer_(const std::vector<column> &metadata) {
    /// <i>Initialize the unflat pbuffer based on a metadata vector</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>std::vector<column> &metadata:&emsp; metadata vector</li>
    /// </ul>
    for(size_t idx=0; idx < CONSTS::BUFF_COUNT; idx++) {

        pbuffer_[idx].clear();
        const size_t I = metadata.size();
        pbuffer_[idx].resize(I);
        for (size_t i = 0; i < I; i++) {
            size_t blocks = 1;
            if (metadata[i].nullable) blocks++;
            if (metadata[i].is_true_varchar) blocks++;
            pbuffer_[idx][i].resize(blocks);
        }
        blob_shift_[idx].resize(I);
        for (size_t i = 0; i < I; i++) blob_shift_[idx][i].fill(0);
    }
}

void sqream::driver::flatten_(std::vector<std::vector<std::vector<char>>> &buff_to_flatten) {
    /// <i>Append unflat pbuffer to flat buffer</i><br>
    for(std::vector<std::vector<char>> &cols:buff_to_flatten) for(std::vector<char> &col:cols) buffer_.insert(buffer_.end(),col.begin(),col.end());
}

void sqream::driver::unflatten_() {
    /// <i>Append flat buffer to unflat pbuffer</i><br>
    size_t pos=0,k=0;
    const size_t I=pbuffer_[curr_buff_idx].size();
    for(size_t i=0;i<I;i++)
    {
        const size_t J=pbuffer_[curr_buff_idx][i].size();
        for(size_t j=0;j<J;j++)
        {
            const size_t size=column_sizes_[k++];
            pbuffer_[curr_buff_idx][i][j].resize(size);
            memcpy(pbuffer_[curr_buff_idx][i][j].data(),buffer_.data()+pos,size);
            pos+=size;
        }
    }
}

void sqream::driver::reset_pbuffer_(const std::vector<column> &metadata) {
    for(auto &cols:(pbuffer_[curr_buff_idx])) for(auto &col:cols) col.clear();

    const size_t I=metadata.size();
    blob_shift_[curr_buff_idx].clear();
    blob_shift_[curr_buff_idx].resize(I);
    for(size_t i=0;i<I;i++) blob_shift_[curr_buff_idx][i].fill(0);
}

void sqream::driver::put_buff(size_t row_cnt, int buff_idx) {
    std::unique_lock<std::mutex> lock(buff_switch_mut);
    //std::printf("Will switch from buffer '%d'\n", buff_idx);
    flatten_(pbuffer_[buff_idx]);
    sqc_->put(buffer_, row_cnt);
    //std::printf("put(%ld)\n", ++put_cnt);
    buffer_.clear();
}

void sqream::driver::new_query(const std::string &sql_query) {

    /// <i>This function creates a new statement and deduces its type and metadata</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>const std::string &sql_query:&emsp; SQream SQL Query</li>
    /// </ul>
    TC(sqc_)
    state_=0;
    row_count_=0;
    current_row_=0;
    curr_buff_idx = 0;

    buffer_.clear();
    column_sizes_.clear();
    colck_.clear();
    sqc_->open_statement();
    if(!sqc_->prepare_statement(sql_query,57/*Grothendieck prime*/)) THROW_GENERAL_ERROR("error preparing statement");
    state_|=1;

}

bool sqream::driver::execute_query() {
    /// <i>This function tells the sqreamd server to start executing the newly generated query</i><br>
    /// This function can only be executed after a valid new_query() call<br>
    /// <b>input:</b>
    /// <ul>
    /// <li>const std::string &sql_query:&emsp; SQream SQL Query</li>
    /// </ul>
    TCCS(sqc_,1)
    if(sqc_->execute()) {
        statement_type_=sqc_->metadata_query(metadata_input_,metadata_output_);
        
        switch(statement_type_) {
            case CONSTS::insert: {
                init_pbuffer_(metadata_input_);
                colck_.resize(metadata_input_.size(),0);
            }
            break;
            case CONSTS::select: init_pbuffer_(metadata_output_); break;
            default: break;
        }
        state_|=2;
        return true;
    }
    else 
        THROW_GENERAL_ERROR("failed to execute query");
}


bool sqream::driver::next_query_row(const size_t min_put_size) {

    /// <i>This driver retrieves or sends data per row</i><br>
    /// This function can only be executed after a execute_query() call<br>
    /// <b>input:</b>
    /// <ul>
    /// <li>const size_t &min_put_size:&emsp; minimal size of the binary data black to be sent</li>
    /// </ul>
    TCCS(sqc_,3)
    switch(statement_type_) {
        case CONSTS::insert:
        {
            size_t sum=0;
            for(uint8_t p:colck_) if(p>0) sum++;
            if(sum!=metadata_input_.size()) THROW_GENERAL_ERROR("some columns are unitialized");
            for(uint8_t &p:colck_) p=0;
            if(flat_size_()>=min_put_size)
            {
                if(buffer_switch_th)
                {
                    //std::printf("Ending previous buff switch\n");
                    (*buffer_switch_th).get();
                    buffer_switch_th.reset(nullptr);
                }

                //The launch::async policy here is crucial to be sure it runs right away asynchronously instead of potentially being deferred
                buffer_switch_th.reset(new std::future<void>(std::async(std::launch::async,&sqream::driver::put_buff, this, ++row_count_, curr_buff_idx.load())));
                curr_buff_idx = (curr_buff_idx+1)%CONSTS::BUFF_COUNT;

                reset_pbuffer_(metadata_input_);
                row_count_=0;
            }
            else row_count_++;
            return true;
        }
        case CONSTS::select:
        {
            if(++current_row_<row_count_) return true;
            else
            {
                column_sizes_.clear();
                row_count_=sqc_->fetch(buffer_,column_sizes_);
                if(row_count_)
                {
                    current_row_=0;
                    init_pbuffer_(metadata_output_);
                    unflatten_();
                    buffer_.clear();
                    return true;
                }
                else
                {
                    state_|=4;
                    return false;
                }
            }
        }
        default:
        {
            state_|=4;
            return false;
        }
    }
}

bool sqream::driver::finish_query() {
    /// <i>This driver retrieves or sends data per row</i><br>
    /// This function can only be executed after a execute_query() call
    if(state_==3) state_|=4;
    TCCS(sqc_,7)
    if(statement_type_==CONSTS::insert) {
        if(buffer_switch_th) {
            //std::printf("Ending previous buff switch\n");
            (*buffer_switch_th).get();
            buffer_switch_th.reset(nullptr);
        }
        if(flat_size_()) {
            put_buff(row_count_, curr_buff_idx.load());
        }
    }

    state_|=8;
    return sqc_->close_statement();
}

bool sqream::driver::is_null(const size_t col)
{
    /// <i>check if a value is nullefied</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>const size_t &col:&emsp; column index</li>
    /// </ul>
    /// <b>return</b>(bool):&emsp; value
    TCCSCO(sqc_,3,col)
    if(!is_nullable(col)) THROW_GENERAL_ERROR("column is not nullable");
    bool retval;
    memcpy(&retval,pbuffer_[curr_buff_idx][col][0].data()+current_row_,sizeof(retval));
    if(metadata_output_[col].type == "ftBlob" && retval == true)
    {
        const size_t idx=current_row_%2;
        blob_shift_[curr_buff_idx][col][1-idx]=blob_shift_[curr_buff_idx][col][idx];
    }
    return retval;
}

bool sqream::driver::is_nullable(const size_t col)
{
    /// <i>check if a value is nullified</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>const size_t &col:&emsp; column index</li>
    /// </ul>
    /// <b>return</b>(bool):&emsp; value
    switch(statement_type_)
    {
        case CONSTS::insert:
        {
            TCCSCI(sqc_,3,col)
            return metadata_input_[col].nullable;
        }
        case CONSTS::select:
        {
            TCCSCO(sqc_,3,col)
            return metadata_output_[col].nullable;
        }
        default: THROW_GENERAL_ERROR("statement does not support metadata queries");
    }
}

/*!
\def GET_FIXED_TYPES(X,Y,Z)
<i>This macro implements all <b>get</b> functions for fixed types</i>
<b>input:</b>
<ul>
<li>\a X:&emsp; SQream type name</li>
<li>\a Y:&emsp; THROW type name</li>
<li>\a Z:&emsp; C++ type name</li>
</ul>
*/
#define GET_FIXED_TYPES(X,Y,Z)\
{\
    TCCSCO(sqc_,3,col)\
    if(metadata_output_[col].type!=#X) THROW_GENERAL_ERROR("column is not of type "#Y);\
    const size_t id=metadata_output_[col].nullable?1:0;\
    const size_t shift=metadata_output_[col].size*current_row_;\
    Z retval;\
    memcpy(&retval,pbuffer_[curr_buff_idx][col][id].data()+shift,sizeof(retval));\
    return retval;\
}
bool sqream::driver::get_bool(const size_t col) GET_FIXED_TYPES(ftBool,bool,bool)
///< <i>retrieve a boolean type value from a column by index</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(bool):&emsp; value
uint8_t sqream::driver::get_ubyte(const size_t col) GET_FIXED_TYPES(ftUByte,UByte,uint8_t)
///< <i>retrieve a unsigned byte type value from a column by index</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(uint8_t):&emsp; value
int16_t sqream::driver::get_short(const size_t col) GET_FIXED_TYPES(ftShort,short,int16_t)
///< <i>retrieve a short type value from a column by index</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(int16_t):&emsp; value
int32_t sqream::driver::get_int(const size_t col) GET_FIXED_TYPES(ftInt,int,int32_t)
///< <i>retrieve a int type value from a column by index</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(int32_t):&emsp; value
int64_t sqream::driver::get_long(const size_t col) GET_FIXED_TYPES(ftLong,long,int64_t)
///< <i>retrieve a long type value from a column by index</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(int64_t):&emsp; value
float sqream::driver::get_float(const size_t col) GET_FIXED_TYPES(ftFloat,float,float)
///< <i>retrieve a float type value from a column by index</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(float):&emsp; value
double sqream::driver::get_double(const size_t col) GET_FIXED_TYPES(ftDouble,double,double)
///< <i>retrieve a double type value from a column by index</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(double):&emsp; value
uint32_t sqream::driver::get_date(const size_t col) GET_FIXED_TYPES(ftDate,date,uint32_t)
///< <i>retrieve a date type value from a column by index</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(uint32_t):&emsp; value
uint64_t sqream::driver::get_datetime(const size_t col) GET_FIXED_TYPES(ftDateTime,datetime,uint64_t)
///< <i>retrieve a datetime type value from a column by index</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(uint64_t):&emsp; value
#undef GET_FIXED_TYPES

std::string sqream::driver::get_varchar(const size_t col)
{
    /// <i>retrieve a varchar type value from a column by index</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>const size_t &col:&emsp; column index</li>
    /// </ul>
    /// <b>return</b>(std::string):&emsp; value
    TCCSCO(sqc_,3,col)
    if(metadata_output_[col].type!="ftVarchar") THROW_GENERAL_ERROR("column is not of type varchar");
    const size_t id=metadata_output_[col].nullable?1:0;
    const size_t size=metadata_output_[col].size;
    const size_t shift=metadata_output_[col].size*current_row_;
    std::string retval;
    retval.resize(size);
    memcpy(const_cast<char*>(retval.data()),pbuffer_[curr_buff_idx][col][id].data()+shift,size);
    return retval;
}

std::string sqream::driver::get_nvarchar(const size_t col)
{
    /// <i>retrieve a nvarchar type value from a column by index</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>const size_t &col:&emsp; column index</li>
    /// </ul>
    /// <b>return</b>(std::string):&emsp; value
    TCCSCO(sqc_,3,col)
    if(metadata_output_[col].type!="ftBlob") THROW_GENERAL_ERROR("column is not of type nvarchar");
    const size_t ids=metadata_output_[col].nullable?1:0;
    const size_t idn=ids+1;
    const size_t shift=4*current_row_;
    const size_t idx=current_row_%2;
    int nvarchar_size_container;
    memcpy(&nvarchar_size_container,pbuffer_[curr_buff_idx][col][ids].data()+shift,sizeof(nvarchar_size_container));
    std::string retval;
    retval.resize(nvarchar_size_container);
    memcpy(const_cast<char*>(retval.data()),pbuffer_[curr_buff_idx][col][idn].data()+blob_shift_[curr_buff_idx][col][idx],nvarchar_size_container);
    blob_shift_[curr_buff_idx][col][1-idx]=blob_shift_[curr_buff_idx][col][idx]+nvarchar_size_container;
    return retval;
}

/*!
\def NAMED_GETS(X)
<i>This macro implements all <b>get</b> calls by column name</i>
<b>input:</b>
<ul>
<li>\a X:&emsp; function name by column index</li>
</ul>
*/
#define NAMED_GETS(X)\
{\
    const size_t I=metadata_output_.size();\
    for(size_t i=0;i<I;i++) if(metadata_output_[i].name==col_name) return X(i);\
    THROW_GENERAL_ERROR("column name not found");\
}

bool sqream::driver::is_nullable(const std::string &col_name) NAMED_GETS(is_nullable)
///< <i>retrieve if column is nullable by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(bool):&emsp; value
bool sqream::driver::is_null(const std::string &col_name) NAMED_GETS(is_null)
///< <i>retrieve if column value is null by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(bool):&emsp; value
bool sqream::driver::get_bool(const std::string &col_name) NAMED_GETS(get_bool)
///< <i>retrieve a bool type value from a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(bool):&emsp; value
uint8_t sqream::driver::get_ubyte(const std::string &col_name) NAMED_GETS(get_ubyte)
///< <i>retrieve a ubyte type value from a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(uint8_t):&emsp; value
int16_t sqream::driver::get_short(const std::string &col_name) NAMED_GETS(get_short)
///< <i>retrieve a short type value from a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(int16_t):&emsp; value
int32_t sqream::driver::get_int(const std::string &col_name) NAMED_GETS(get_int)
///< <i>retrieve a int type value from a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(int32_t):&emsp; value
int64_t sqream::driver::get_long(const std::string &col_name) NAMED_GETS(get_long)
///< <i>retrieve a long type value from a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(int64_t):&emsp; value
float sqream::driver::get_float(const std::string &col_name) NAMED_GETS(get_float)
///< <i>retrieve a float type value from a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(float):&emsp; value
double sqream::driver::get_double(const std::string &col_name) NAMED_GETS(get_double)
///< <i>retrieve a double type value from a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(double):&emsp; value
uint32_t sqream::driver::get_date(const std::string &col_name) NAMED_GETS(get_date)
///< <i>retrieve a date type value from a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(date):&emsp; value
uint64_t sqream::driver::get_datetime(const std::string &col_name) NAMED_GETS(get_datetime)
///< <i>retrieve a datetime type value from a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(datetime):&emsp; value
std::string sqream::driver::get_varchar(const std::string &col_name) NAMED_GETS(get_varchar)
///< <i>retrieve a varchar type value from a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(std::string):&emsp; value
std::string sqream::driver::get_nvarchar(const std::string &col_name) NAMED_GETS(get_nvarchar)
///< <i>retrieve a nvarchar type value from a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< </ul>
///< <b>return</b>(std::string):&emsp; value
#undef NAMED_GETS

/// Macro to check if a column is already checked
#define COLCK if(!colck_[col]) colck_[col]=1; \
else THROW_GENERAL_ERROR("column already set");\

/// Macro to set a false value to the NULL column if present
#define NULL_WHIPER if(metadata_input_[col].nullable)\
{\
    const bool whiper=false;\
    const char * const wptr=(char*)&whiper;\
    pbuffer_[curr_buff_idx][col][0].insert(pbuffer_[curr_buff_idx][col][0].end(),wptr,wptr+sizeof(whiper));\
}

void sqream::driver::set_null(const size_t col)
{
    /// <i>nullify value of a column by index</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>const size_t &col:&emsp; column index</li>
    /// <li>const bool &col:&emsp; value</li>
    /// </ul>
    TCCSCI(sqc_,3,col)
    if(!is_nullable(col)) THROW_GENERAL_ERROR("column is not nullable");
    COLCK
    const bool value=true;
    const char * const ptr=(char*)&value;
    pbuffer_[curr_buff_idx][col][0].insert(pbuffer_[curr_buff_idx][col][0].end(),ptr,ptr+sizeof(value));
    std::vector<char> whiper(metadata_input_[col].is_true_varchar?4:metadata_input_[col].size,metadata_input_[col].name=="ftVarchar"?32:0);
    pbuffer_[curr_buff_idx][col][1].insert(pbuffer_[curr_buff_idx][col][1].end(),whiper.begin(),whiper.end());
}

/*!
\def SET_FIXED_TYPES(X,Y)
<i>This macro implements all <b>set</b> functions for fixed types</i>
<b>input:</b>
<ul>
<li>\a X:&emsp; SQream type name</li>
<li>\a Y:&emsp; C++ type name</li>
</ul>
*/
#define SET_FIXED_TYPES(X,Y)\
{\
    TCCSCI(sqc_,3,col)\
    if(metadata_input_[col].type!=#X) THROW_GENERAL_ERROR("column is not of type "#Y);\
    COLCK \
    const size_t id=metadata_input_[col].nullable?1:0;\
    const char * const ptr=(char*)&value;\
    pbuffer_[curr_buff_idx][col][id].insert(pbuffer_[curr_buff_idx][col][id].end(),ptr,ptr+sizeof(value));\
    NULL_WHIPER\
}
void sqream::driver::set_bool(const size_t col,const bool value) SET_FIXED_TYPES(ftBool,bool)
///< <i>set a bool type value of a column by index</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const bool &value:&emsp; value</li>
///< </ul>
void sqream::driver::set_ubyte(const size_t col,const uint8_t value) SET_FIXED_TYPES(ftUByte,UByte)
///< <i>set a unsigned byte type value of a column by index</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const int8_t &value:&emsp; value</li>
///< </ul>
void sqream::driver::set_short(const size_t col,const uint16_t value) SET_FIXED_TYPES(ftShort,short)
///< <i>set a short type value of a column by index</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const int16_t &value:&emsp; value</li>
///< </ul>
void sqream::driver::set_int(const size_t col,const uint32_t value) SET_FIXED_TYPES(ftInt,int)
///< <i>set a int type value of a column by index</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const int32_t &col:&emsp; value</li>
///< </ul>
void sqream::driver::set_long(const size_t col,const uint64_t value) SET_FIXED_TYPES(ftLong,long)
///< <i>set a long type value of a column by index</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const int64_t &value:&emsp; value</li>
///< </ul>
void sqream::driver::set_float(const size_t col,const float value) SET_FIXED_TYPES(ftFloat,float)
///< <i>set a float type value of a column by index</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const float &value:&emsp; value</li>
///< </ul>
void sqream::driver::set_double(const size_t col,const double value) SET_FIXED_TYPES(ftDouble,double)
///< <i>set a double type value of a column by index</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const double &value:&emsp; value</li>
///< </ul>
void sqream::driver::set_date(const size_t col,const uint32_t value) SET_FIXED_TYPES(ftDate,date)
///< <i>set a date type value of a column by index</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const date &value:&emsp; value</li>
///< </ul>
void sqream::driver::set_datetime(const size_t col,const uint64_t value) SET_FIXED_TYPES(ftDateTime,datetime)
///< <i>set a datetime type value of a column by index</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const datetime &value:&emsp; value</li>
///< </ul>
#undef SET_FIXED_TYPES

void sqream::driver::set_varchar(const size_t col,const std::string &value)
{
    ///< <i>set a varchar type value of a column by index</i><br>
    ///< <b>input:</b>
    ///< <ul>
    ///< <li>const size_t &col:&emsp; column index</li>
    ///< <li>const std::string &value:&emsp; value</li>
    ///< </ul>
    TCCSCI(sqc_,3,col)
    if(metadata_input_[col].type!="ftVarchar") THROW_GENERAL_ERROR("column is not of type varchar");
    if(metadata_input_[col].size<value.size()) THROW_GENERAL_ERROR("string size is bigger than column varchar size");
    COLCK
    const size_t id=metadata_input_[col].nullable?1:0;
    const char * const ptr=value.c_str();
    pbuffer_[curr_buff_idx][col][id].insert(pbuffer_[curr_buff_idx][col][id].end(),ptr,ptr+value.size());
    std::vector<char> spaces(metadata_input_[col].size-value.size(),' ');
    pbuffer_[curr_buff_idx][col][id].insert(pbuffer_[curr_buff_idx][col][id].end(),spaces.begin(),spaces.end());
    NULL_WHIPER
}

void sqream::driver::set_nvarchar(const size_t col,const std::string &value)
{
    ///< <i>set a nvarchar type value of a column by index</i><br>
    ///< <b>input:</b>
    ///< <ul>
    ///< <li>const size_t &col:&emsp; column index</li>
    ///< <li>const std::string &value:&emsp; value</li>
    ///< </ul>
    TCCSCI(sqc_,3,col)
    if(metadata_input_[col].type!="ftBlob" and !metadata_input_[col].is_true_varchar) THROW_GENERAL_ERROR("column is not of type nvarchar");
    COLCK
    const size_t ids=metadata_input_[col].nullable?1:0;
    const size_t idn=ids+1;
    const int nvarchar_size_container=value.size();
    const char * const size_ptr=(const char *)&nvarchar_size_container;
    pbuffer_[curr_buff_idx][col][ids].insert(pbuffer_[curr_buff_idx][col][ids].end(),size_ptr,size_ptr+sizeof(nvarchar_size_container));
    const char * const ptr=value.c_str();
    pbuffer_[curr_buff_idx][col][idn].insert(pbuffer_[curr_buff_idx][col][idn].end(),ptr,ptr+value.size());
    NULL_WHIPER
}

void sqream::driver::set_null(const std::string &col_name)
{
    /// <i>nullify value of a column by name</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>const size_t &col:&emsp; column index</li>
    /// <li>const bool &col:&emsp; value</li>
    /// </ul>
    (void) col_name;
    THROW_GENERAL_ERROR("setting for named columns is currently not supported");
}

/*!
\def NAMED_SETS
<i>This macro implements all <b>set</b> calls by column name</i>
*/
#define NAMED_SETS \
{\
    (void) col_name;\
    (void) value;\
    THROW_GENERAL_ERROR("setting for named columns is currently not supported");\
}
void sqream::driver::set_bool(const std::string &col_name,const bool value) NAMED_SETS
///< <i>set a bool type value of a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const bool &value:&emsp; value</li>
///< </ul>
void sqream::driver::set_ubyte(const std::string &col_name,const uint8_t value) NAMED_SETS
///< <i>set a unsigned byte type value of a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const uint8_t &value:&emsp; value</li>
///< </ul>
void sqream::driver::set_short(const std::string &col_name,const uint16_t value) NAMED_SETS
///< <i>set a short type value of a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const int16_t &value:&emsp; value</li>
///< </ul>
void sqream::driver::set_int(const std::string &col_name,const uint32_t value) NAMED_SETS
///< <i>set a int type value of a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const int32_t &value:&emsp; value</li>
///< </ul>
void sqream::driver::set_long(const std::string &col_name,const uint64_t value) NAMED_SETS
///< <i>set a long type value of a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const int64_t &value:&emsp; value</li>
///< </ul>
void sqream::driver::set_float(const std::string &col_name,const float value) NAMED_SETS
///< <i>set a float type value of a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const float &value:&emsp; value</li>
///< </ul>
void sqream::driver::set_double(const std::string &col_name,const double value) NAMED_SETS
///< <i>set a double type value of a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const double &value:&emsp; value</li>
///< </ul>
void sqream::driver::set_date(const std::string &col_name,const uint32_t value) NAMED_SETS
///< <i>set a date type value of a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const uint32_t &value:&emsp; value</li>
///< </ul>
void sqream::driver::set_datetime(const std::string &col_name,const uint64_t value) NAMED_SETS
///< <i>set a datetime type value of a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const uint64_t &value:&emsp; value</li>
///< </ul>
void sqream::driver::set_varchar(const std::string &col_name,const std::string &value) NAMED_SETS
///< <i>set a varchar type value of a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const std::string &value:&emsp; value</li>
///< </ul>
void sqream::driver::set_nvarchar(const std::string &col_name,const std::string &value) NAMED_SETS
///< <i>set a nvarchar type value of a column by name</i><br>
///< <b>input:</b>
///< <ul>
///< <li>const size_t &col:&emsp; column index</li>
///< <li>const std::string &value:&emsp; value</li>
///< </ul>
#undef NAMED_SETS

uint32_t sqream::date_t::get()
{
    const int32_t m=(month + 9)%12;
    const int32_t y=year-m/10;
    return 365*y+y/4-y/100+y/400+(m*306+5)/10+(day-1);
}

void sqream::date_t::set(uint32_t date)
{
    const int64_t dlong=(int64_t)(*(int32_t*)&date);
    #define dform dlong-(y*365+y/4-y/100+y/400)
    int64_t y=(10000*dlong+14780)/3652425;
    int64_t d=dform;
    if(d<0)
    {
       y--;
       d=dform;
    }
    #undef dform
    const int64_t m=(52+100*d)/3060;
    year=y+(m+2)/12;
    month=(m+2)%12+1;
    day=d-(m*306+5)/10+1;
    if(!validate()) THROW_GENERAL_ERROR("invalid date format was set");

}

bool sqream::date_t::validate()
{
    const int leap[]={31,29,31,30,31,30,31,31,30,31,30,31};
    const int noleap[]={31,28,31,30,31,30,31,31,30,31,30,31};
    if(year<0 or year>10000 or month<1 or month>12) return false;
    if(day<1 or day>((year%400==0 or (year%4==0 and year%100!=0))?leap[month-1]:noleap[month-1])) return false;
    return true;
}

uint64_t sqream::datetime_t::get()
{
    date_t date;
    date.year=year;
    date.month=month;
    date.day=day;
    return (((uint64_t)date.get())<<32)+3600000*hour+60000*minute+1000*second+millisecond;
}

void sqream::datetime_t::set(uint64_t datetime) {

    date_t date;
    date.set((uint32_t)(datetime>>32));
    year=date.year;
    month=date.month;
    day=date.day;
    const uint32_t time=datetime&0xFFFFFFFF;
    millisecond=time%1000;
    second=(time/1000)%60;
    minute=(time/60000)%60;
    hour=time/3600000;
    if(!validate()) THROW_GENERAL_ERROR("invalid datetime format was set");
}


bool sqream::datetime_t::validate() {

    date_t date;
    date.set((uint32_t)(this->get()>>32));
    if(!date.validate()) return false;
    if(millisecond<0 or millisecond>999) return false;
    if(second<0 or second>59) return false;
    if(minute<0 or minute>59) return false;
    if(hour<0 or hour>23) return false;
    return true;
}


uint32_t sqream::date(int32_t year,int32_t month,int32_t day) {

    date_t d;
    d.year=year;
    d.month=month;
    d.day=day;
    return d.get();
}


uint64_t sqream::datetime(int32_t year,int32_t month,int32_t day,int32_t hour,int32_t minute,int32_t second,int32_t millisecond) {
   
    datetime_t dt;
    dt.year=year;
    dt.month=month;
    dt.day=day;
    dt.hour=hour;
    dt.minute=minute;
    dt.second=second;
    dt.millisecond=millisecond;
    return dt.get();
}

sqream::date_t sqream::make_date(uint32_t date) {

    date_t retval;
    retval.set(date);
    return retval;
}

sqream::datetime_t sqream::make_datetime(uint64_t datetime) {

    datetime_t retval;
    retval.set(datetime);
    return retval;
}


void sqream::new_query_execute(driver *drv, std::string sql_query) {
    /// <i>operates the protocol using a connector driver to prepare and execute a query but stops before closing to permit fetching or putting (enables networking insert)</i><br>
    /// <b>input:</b>
    /// <ul>
    /// <li>driver &col:&emsp; column index</li>
    /// <li>driver &col:&emsp; column index</li>
    /// </ul>
    TC(drv->sqc_)
    try{
        drv->new_query(sql_query);
        drv->execute_query();
    }
    catch(std::string &err)
    {
        drv->state_=15;
        drv->sqc_->close_statement();
        THROW_GENERAL_ERROR(err.c_str());
    }
}


void sqream::run_direct_query(driver *drv,std::string sql_query) {
    
    if(not drv) 
        THROW_GENERAL_ERROR("sqream driver is not initialized");

    try {
        new_query_execute(drv,sql_query);
        drv->finish_query();
    }
    catch(std::string &err) {
        THROW_GENERAL_ERROR(err.c_str());
    }
}


std::vector<sqream::column> sqream::get_metadata(driver *drv) {

    if(!drv->sqc_) THROW_GENERAL_ERROR("sqream driver is not connected");
    if(drv->state_<1) THROW_GENERAL_ERROR("no metadata has been retrieved at this stage");
    std::vector<column> retval;
    switch(drv->statement_type_)
    {
        case CONSTS::insert: retval=drv->metadata_input_; break;
        case CONSTS::select: retval=drv->metadata_output_; break;
        default: break;
    }
    return retval;
}


uint32_t sqream::retrieve_statement_id(driver *drv) {

    if(!drv->sqc_) THROW_GENERAL_ERROR("sqream driver is not connected");
    if(drv->state_<1) THROW_GENERAL_ERROR("no statement id has been retrieved at this stage");
    return drv->sqc_->statement_id_;
}


sqream::CONSTS::statement_type sqream::retrieve_statement_type(driver *drv) {

    if(!drv->sqc_) THROW_GENERAL_ERROR("sqream driver is not connected");
    if(drv->state_<3) THROW_GENERAL_ERROR("no statement type has been retrieved at this stage");
    return drv->statement_type_;
}

#undef THROW_GENERAL_ERROR
#undef THROW_SQREAM_ERROR
