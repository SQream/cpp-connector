#ifndef __SQream_cpp_connector_h__
#define __SQream_cpp_connector_h__
#ifndef __linux__
#define __func__ __FUNCTION__
#include <ciso646>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <future>
#include <mutex>
#include <memory>
#include <atomic>

#define CPPCONECTOR_MAJOR_VERSION 4
#define CPPCONECTOR_MINOR_VERSION 0
#define CPPCONECTOR_PATCH_VERSION 0
#define CPPCONECTOR_VERSION_STRING std::string(std::to_string(CPPCONECTOR_MAJOR_VERSION) + "." + std::to_string(CPPCONECTOR_MINOR_VERSION) + "." + std::to_string(CPPCONECTOR_PATCH_VERSION))

/// <h3>SQream low-level connector main namespace</h3>
struct TSocketClient;

namespace sqream
{
    /// <h3>sqream::HEADER contains all the information for the headers</h3>
    namespace HEADER
    {
        const uint8_t PROTOCOL_VERSION=8;                                           ///< Current protocol version
        const uint8_t TYPE_JSON=1;                                                  ///< JSON message identifier
        const uint8_t TYPE_BINARY=2;                                                ///< Binary message identifier
        const uint8_t SIZE=2;                                                       ///< PRE_HEADER size
        const uint8_t HEADER_JSON[SIZE]={PROTOCOL_VERSION,TYPE_JSON};               ///< JSON PRE_HEADER
        const uint8_t HEADER_BINARY[SIZE]={PROTOCOL_VERSION,TYPE_BINARY};           ///< Binary PRE_HEADER
    }
    /// <h3>sqream::CONSTS contains all the SQream defined constants</h3>
    namespace CONSTS
    {
        const uint8_t BUFF_COUNT=2;
        const char DEFAULT_SERVICE[]="sqream";
        const uint32_t MAX_SIZE=1<<30;                                              ///< Maximum message size (2^30 Byte = 1073741824 Byte = 1 GiB)
        const uint32_t MIN_PUT_SIZE=1<<26;                                          ///< Default minimum buffer size (2^26 Byte = 67108864 Byte = 64 MiB)
        /// <h3>statement operation types char enum</h3>
        enum statement_type:char
        {
            unset,                                                                      ///< Undefined
            select,                                                                     ///< Select type
            insert,                                                                     ///< Insert type
            direct,                                                                     ///< Direct type (not an insert nor a select)
        };
    }
    /// <h3>formated and unformated JSON message cstrings that can be sent to a sqreamd instance</h3>
    namespace MESSAGES
    {
#define JS1(x) "{\""#x"\":\""#x"\"}"
#define JSA(...) "{"#__VA_ARGS__"}"
        const char closeConnection[]=JS1(closeConnection);                                              ///< Definition of closeConnection message
        const char closeStatement[]=JS1(closeStatement);                                                ///< Definition of closeStatement message
        const char connectDatabase[]=JSA("service":"%s","username":"%s","password":"%s","connectDatabase":"%s");///< Definition of connectDatabase UNFORMATTED message
        const char reconnectDatabase[]=JSA("reconnectDatabase":"%s","service":"%s","connectionId":%u,"username":"%s","password":"%s","listenerId":%u);///< Definition of reconnectDatabase UNFORMATTED message
        const char getStatementId[]=JS1(getStatementId);                                                ///< Definition of getStatementId message
        const char prepareStatement[]=JSA("prepareStatement":"%s","chunkSize":%u);                      ///< Definition of prepareStatement UNFORMATTED message
        const char reconstructStatement[]=JSA("reconstructStatement":%u);                               ///< Definition of reconstructStatemnt UNFORMATTED message
        const char queryTypeOut[]=JS1(queryTypeOut);                                                    ///< Definition of queryTypeOut message
        const char queryTypeIn[]=JS1(queryTypeIn);                                                      ///< Definition of queryTypeIn message
        const char execute[]=JS1(execute);                                                              ///< Definition of execute message
        const char fetch[]=JS1(fetch);                                                                  ///< Definition of fetch message
        const char put[]=JSA("put":%u);                                                                 ///< Definition of put UNFORMATTED message
#undef JS1
#undef JSA
        template<typename ...Args> void format(std::vector<char> &output,const char input[],Args...args);   ///< <h3>Unformatted message formatter (snprintf-wrapper)</h3>
    }

    struct column {
        std::string name;                                                                               ///< <h3>Column name</h3>
        bool nullable;                                                                                  ///< <h3>Nullability of a column</h3>
        bool is_true_varchar;                                                                           ///< <h3>Is the column a true varchar</h3>
        std::string type;                                                                               ///< <h3>Sqream datatype of column</h3>
        unsigned size;                                                                                  ///< <h3>Size in bytes of sqream datatype</h3>
        unsigned scale;                                                                                 ///< <h3>Scale of chunk</h3>
    };

    /// <h3>Low level connector</h3>
    struct connector {
        TSocketClient *socket;  
        std::string ipv4_;                                                                                                          ///< <h3>Newest ip4v address</h3> (internal)
        int port_;                                                                                                                  ///< <h3>Newest port</h3> (internal)
        bool ssl_;
        std::string username_;
        std::string password_;
        std::string database_;
        std::string service_;
        std::string var_encoding_;
        uint32_t connection_id_;                                                                                                    ///< <h3>Newest connection id</h3> (internal)
        uint32_t statement_id_;                                                                                                     ///< <h3>Newest statement id</h3> (internal)
        connector();                                                                                                                ///< <h3>Trivial constructor</h3>
        ~connector();     
        void connect_socket(const std::string &ipv4,int port,bool ssl);
        void read  (std::vector<char> &data);
        void write (const char *data,const uint64_t data_size,const uint8_t msg_type[HEADER::SIZE]);
        bool connect(const std::string &ipv4,int port,bool ssl,const std::string &username,const std::string &password,const std::string &database,const std::string &service); ///< <h3>Manual connection message</h3>
        bool reconnect(const std::string &ipv4,int port,int listener_id);                                                            ///< <h3>(Load Balancer) Reconnect to a sqreamd instance</h3>
        bool open_statement();                                                                                                      ///< <h3>Open a new statement message</h3>
        bool prepare_statement(std::string sqlQuery,int chunk_size);                                                                ///< <h3>Prepare a new sql query message</h3>
        CONSTS::statement_type metadata_query(std::vector<column> &columns_metadata_in,std::vector<column> &columns_metadata_out);  ///< <h3>Retrieve input/output metadata message</h3>
        bool execute();                                                                                                             ///< <h3>Execute statement message</h3>
        size_t fetch(std::vector<char> &binary_data,std::vector<uint64_t> &column_sizes,size_t min_size=1);                         ///< <h3>Retrieve raw data from server message</h3>
        void put(std::vector<char> &binary_data,size_t rows);                                                                       ///< <h3>Insert raw data to server message</h3>
        bool close_statement();                                                                                                     ///< <h3>Close a statement message</h3>
#undef ERR_HANDLE
#undef ERR_HANDLE_STR
    };
    
    /// <h3>SQream high level connector (driver)</h3>
    struct driver {
        std::unique_ptr<std::future<void>> buffer_switch_th;
        std::mutex buff_switch_mut;
        std::atomic<int> curr_buff_idx;
        connector *sqc_;                                                                                                            ///< <h3>SQream low level connector pointer</h3> (internal)
        uint32_t listener_id_;
        std::string statement_;                                                                                                     ///< <h3>Newest statement</h3> (internal)
        CONSTS::statement_type statement_type_;                                                                                     ///< <h3>Newest statement type</h3> (internal)
        std::vector<column> metadata_input_;                                                                                        ///< <h3>Column metadata info for network insert</h3> (internal)
        std::vector<column> metadata_output_;                                                                                       ///< <h3>Column metadata info for select</h3> (internal)
        std::vector<uint64_t> column_sizes_;                                                                                        ///< <h3>Retrieved column size of last fetch</h3> (internal)
        std::vector<char> buffer_;                                                                                                  ///< <h3>Flattened data buffer</h3> (internal)
        std::vector<std::vector<std::vector<char>>> pbuffer_[CONSTS::BUFF_COUNT];                                                                       ///< <h3>Unflattened data buffer</h3> (internal)
        size_t row_count_;                                                                                                          ///< <h3>Rows retrieved/inserted</h3> (internal)
        size_t current_row_;                                                                                                        ///< <h3>Row that is currently manipulated by set/get functions</h3> (internal)
        std::vector<std::array<size_t,2>> blob_shift_[CONSTS::BUFF_COUNT];                                                                              ///< <h3>Size counter of read nvarchars per row</h3> (internal)
        uint8_t state_;                                                                                                             ///< <h3>Checksum of state of the structure</h3> (internal)
        std::vector<uint8_t> colck_;                                                                                                ///< <h3>Counter for every set column</h3> (internal)
        driver();                                                                                                                   ///< <h3>Constructor</h3>
        ~driver();                                                                                                                  ///< <h3>Destructor</h3>
        size_t flat_size_();                                                                                                        ///< <h3>Size of flat buffer</h3> (internal)
        void init_pbuffer_(const std::vector<column> &metadata);                                                                    ///< <h3>Initializer for unflattend buffer</h3> (internal)
        void reset_pbuffer_(const std::vector<column> &metadata);
        void put_buff(size_t row_cnt, int buff_idx);
        void flatten_(std::vector<std::vector<std::vector<char>>> &buff_to_flatten);                                                                                                            ///< <h3>Copy data from unflattend buffer to flat buffer</h3> (internal)
        void unflatten_();                                                                                                          ///< <h3>Copy data from flat buffer to unflattend buffer</h3> (internal)
        bool connect(const std::string &ipv4,int port,bool ssl,const std::string &username,const std::string &password,const std::string &database,const std::string &service=std::string(CONSTS::DEFAULT_SERVICE));        ///< <h3>Connect to a sqreamd instance</h3>
        void disconnect();                                                                                                          ///< <h3>Disconnect to sqreamd instance</h3>
        void new_query(const std::string &sql_query);                                                                               ///< <h3>Create a new SQream query</h3>
        bool execute_query();                                                                                                       ///< <h3>Execute the current query</h3>
        bool next_query_row(const size_t min_put_size=CONSTS::MIN_PUT_SIZE);                                                        ///< <h3>Move to next row</h3>
        bool finish_query();                                                                                                        ///< <h3>Finish the current query</h3>
        bool is_nullable(const size_t col);                                                                                         ///< <h3>Check column is nullable by column index</h3>
        bool is_null(const size_t col);                                                                                             ///< <h3>Check nullity of selected row by column index</h3>
        bool get_bool(const size_t col);                                                                                            ///< <h3>Get boolean value of selected row by column index</h3>
        uint8_t get_ubyte(const size_t col);                                                                                        ///< <h3>Get unsigned byte value of selected row by column index</h3>
        int16_t get_short(const size_t col);                                                                                        ///< <h3>Get short value of selected row by column index</h3>
        int32_t get_int(const size_t col);                                                                                          ///< <h3>Get int value of selected row by column index</h3>
        int64_t get_long(const size_t col);                                                                                         ///< <h3>Get long value of selected row by column index</h3>
        float get_float(const size_t col);                                                                                          ///< <h3>Get float value of selected row by column index</h3>
        double get_double(const size_t col);                                                                                        ///< <h3>Get double value of selected row by column index</h3>
        uint32_t get_date(const size_t col);                                                                                        ///< <h3>Get date value of selected row by column index</h3>
        uint64_t get_datetime(const size_t col);                                                                                    ///< <h3>Get datetime value of selected row by column index</h3>
        std::string get_varchar(const size_t col);                                                                                  ///< <h3>Get varchar value of selected row by column index</h3>
        std::string get_nvarchar(const size_t col);                                                                                 ///< <h3>Get nvarchar value of selected row by column index</h3>
        bool is_nullable(const std::string &col_name);                                                                              ///< <h3>Check column is nullable by column index</h3>
        bool is_null(const std::string &col_name);                                                                                  ///< <h3>Check nullity of selected row by column name</h3>
        bool get_bool(const std::string &col_name);                                                                                 ///< <h3>Get boolean value of selected row by column name</h3>
        uint8_t get_ubyte(const std::string &col_name);                                                                             ///< <h3>Get unsigned byte value of selected row by column name</h3>
        int16_t get_short(const std::string &col_name);                                                                             ///< <h3>Get short value of selected row by column name</h3>
        int32_t get_int(const std::string &col_name);                                                                               ///< <h3>Get int value of selected row by column name</h3>
        int64_t get_long(const std::string &col_name);                                                                              ///< <h3>Get long value of selected row by column name</h3>
        float get_float(const std::string &col_name);                                                                               ///< <h3>Get float value of selected row by column name</h3>
        double get_double(const std::string &col_name);                                                                             ///< <h3>Get double value of selected row by column name</h3>
        uint32_t get_date(const std::string &col_name);                                                                             ///< <h3>Get date value of selected row by column name</h3>
        uint64_t get_datetime(const std::string &col_name);                                                                         ///< <h3>Get datetime value of selected row by column name</h3>
        std::string get_varchar(const std::string &col_name);                                                                       ///< <h3>Get varchar value of selected row by column name</h3>
        std::string get_nvarchar(const std::string &col_name);                                                                      ///< <h3>Get nvarchar value of selected row by column name</h3>
        void set_null(const size_t col);                                                                                            ///< <h3>Set nullity of insertion row by column index</h3>
        void set_bool(const size_t col,const bool value);                                                                           ///< <h3>Set boolean value of insertion row by column index</h3>
        void set_ubyte(const size_t col,const uint8_t value);                                                                       ///< <h3>Set unsigned byte value of insertion row by column index</h3>
        void set_short(const size_t col,const uint16_t value);                                                                      ///< <h3>Set short value of insertion row by column index</h3>
        void set_int(const size_t col,const uint32_t value);                                                                        ///< <h3>Set int value of insertion row by column index</h3>
        void set_long(const size_t col,const uint64_t value);                                                                       ///< <h3>Set long value of insertion row by column index</h3>
        void set_float(const size_t col,const float value);                                                                         ///< <h3>Set float value of insertion row by column index</h3>
        void set_double(const size_t col,const double value);                                                                       ///< <h3>Set double value of insertion row by column index</h3>
        void set_date(const size_t col,const uint32_t value);                                                                       ///< <h3>Set date value of insertion row by column index</h3>
        void set_datetime(const size_t col,const uint64_t value);                                                                   ///< <h3>Set datetime value of insertion row by column index</h3>
        void set_varchar(const size_t col,const std::string &value);                                                                ///< <h3>Set varchar value of insertion row by column index</h3>
        void set_nvarchar(const size_t col,const std::string &value);                                                               ///< <h3>Set nvarchar value of insertion row by column index</h3>
        void set_null(const std::string &col_name);                                                                                 ///< <h3>Set nullity of insertion row by column name</h3> (unsupported)
        void set_bool(const std::string &col_name,const bool value);                                                                ///< <h3>Set boolean value of insertion row by column name</h3> (unsupported)
        void set_ubyte(const std::string &col_name,const uint8_t value);                                                            ///< <h3>Set unsigned byte value of insertion row by column name</h3> (unsupported)
        void set_short(const std::string &col_name,const uint16_t value);                                                           ///< <h3>Set short value of insertion row by column name</h3> (unsupported)
        void set_int(const std::string &col_name,const uint32_t value);                                                             ///< <h3>Set int value of insertion row by column name</h3> (unsupported)
        void set_long(const std::string &col_name,const uint64_t value);                                                            ///< <h3>Set long value of insertion row by column name</h3> (unsupported)
        void set_float(const std::string &col_name,const float value);                                                              ///< <h3>Set float value of insertion row by column name</h3> (unsupported)
        void set_double(const std::string &col_name,const double value);                                                            ///< <h3>Set double value of insertion row by column name</h3> (unsupported)
        void set_date(const std::string &col_name,const uint32_t value);                                                            ///< <h3>Set date value of insertion row by column name</h3> (unsupported)
        void set_datetime(const std::string &col_name,const uint64_t value);                                                        ///< <h3>Set datetime value of insertion row by column name</h3> (unsupported)
        void set_varchar(const std::string &col_name,const std::string &value);                                                     ///< <h3>Set varchar value of insertion row by column name</h3> (unsupported)
        void set_nvarchar(const std::string &col_name,const std::string &value);                                                    ///< <h3>Set nvarchar value of insertion row by column name</h3> (unsupported)
    };

    ///< <h3>SQream date conversion structure</h3>
    struct date_t {
        int32_t year;                                                                                                               ///< <h3>Year value</h3>
        int32_t month;                                                                                                              ///< <h3>Month value</h3>
        int32_t day;                                                                                                                ///< <h3>Day value</h3>
        uint32_t get();                                                                                                             ///< <h3>Retrieve date in SQream format</h3>
        void set(uint32_t date);                                                                                                    ///< <h3>Set date in SQream format</h3>
        bool validate();                                                                                                            ///< <h3>Validate the current date</h3>
    };

    ///< <h3>SQream datetime conversion structure</h3>
    struct datetime_t {
        int32_t year;                                                                                                               ///< <h3>Year value</h3>
        int32_t month;                                                                                                              ///< <h3>Month value</h3>
        int32_t day;                                                                                                                ///< <h3>Day value</h3>
        int32_t hour;                                                                                                               ///< <h3>Hour value</h3>
        int32_t minute;                                                                                                             ///< <h3>Minute value</h3>
        int32_t second;                                                                                                             ///< <h3>Second value</h3>
        int32_t millisecond;                                                                                                        ///< <h3>Millisecond value</h3>
        uint64_t get();                                                                                                             ///< <h3>Retrieve datetime in SQream format</h3>
        void set(uint64_t datetime);                                                                                                ///< <h3>Set datetime in SQream format</h3>
        bool validate();                                                                                                            ///< <h3>Validate the current datetime</h3>
    };
    uint32_t date(int32_t year,int32_t month,int32_t day);                                                                          ///< <h3>Convert date to sqream date</h3>
    uint64_t datetime(int32_t year,int32_t month,int32_t day,int32_t hour,int32_t minute,int32_t second,int32_t millisecond);       ///< <h3>Convert datetime to sqream datetime</h3>
    date_t make_date(uint32_t date);                                                                                                ///< <h3>Convert sqream date to date_t</h3>
    datetime_t make_datetime(uint64_t datetime);                                                                                    ///< <h3>Convert sqream datetime to datetime_t</h3>
    void new_query_execute(driver *drv,std::string sql_query);                                                                      ///< <h3>Operate the protocol until the statement is executed</h3>
    void run_direct_query(driver *drv,std::string sql_query);                                                                       ///< <h3>Run a direct query without processing input/output</h3>
    std::vector<column> get_metadata(driver *drv);                                                                                  ///< <h3>Return the metadate of the current statement if available</h3>
    uint32_t retrieve_statement_id(driver *drv);
    CONSTS::statement_type retrieve_statement_type(driver *drv);
}
#endif
