#include <limits>
#include <cmath>
#include <iostream>
#include <ostream>
#include <sstream>
#include <regex>
#include <stdlib.h>     /* atoi, srand, rand */
#include <time.h>       /* time */
#include <climits>      /* INT_MAX, LONG_MAX, ... */
#include <limits>
#include <array>
#include <cassert>   // assert()

#include "../connector.h"  // Reflecting the code structure, possibly becomes ../src/connector.h in the future

#define ERR_INTERNAL_RUNTIME            "Internal Runtime Error"
#define ERR_COLUMNS_NOT_SET             "Columns not set"
#define ERR_COLUMN_INDEX_OUT_OF_RANGE   "Column index is out of range"
#define ERR_INVALID_COLUMN_NAME         "Invalid column name"
#define ERR_WRONG_STATEMENT_TYPE        "Wrong statement type"
#define ERR_SQMC                        "SQream-cpp-connector.cc:"

// -- Output macros
#define ping puts("ping");
#define puts(str) puts(str);
#define putss(str) puts(str.c_str());
#define putj(jsn) puts(jsn.dump().c_str());


using namespace std;
namespace con = sqream;


static sqream::driver sqc;

static std::vector<sqream::column> col_input,col_output;
static const string db_name = "network_insert_high_level";


// ----  Connection related   ------
auto& connect(string ip = "127.0.0.1", int port = 5001, bool ssl = true) {
    sqc.connect(ip, port, ssl, "sqream", "sqream", "master");

    return sqc;
} 


//=========TEST TOOLS=========
bool comp_date_t(const sqream::date_t &a, const sqream::date_t &b)
{
    return memcmp(&a, &b, sizeof(sqream::date_t)) == 0;
}

bool comp_datetime_t(const sqream::datetime_t &a, const sqream::datetime_t &b)
{
    return memcmp(&a, &b, sizeof(sqream::datetime_t)) == 0;
}

template <typename L, typename R>
std::ostream &operator<< (std::ostream &s, const std::pair<L, R> &p) {
    s << "(" << p.first << " " << p.second << ")";
    return s;
}

template <typename T>
std::ostream &operator<< (std::ostream &s, const std::vector<T> &v) {
    s << v.size() << " elements {";
    if (!v.empty()) {
        s << v[0];
        for (int i = 1; i < v.size(); ++i) s << ", " << v[i];
    }
    return s << '}';
}

template<typename T>
static void put_in_sstream(std::ostream &s, T&& t) { s << t; }

template<typename T, typename... Args>
static void put_in_sstream(std::ostream &s, T&& t, Args&&... args) {
    s << t;
    put_in_sstream(s, std::forward<Args>(args)...);
}

template<typename... Args>
inline std::string str(Args&&... args) {
    std::stringstream s;
    put_in_sstream(s, std::forward<Args>(args)...);
    return s.str();
}

string rand_utf8_string(int size) {
    if (size <= 0) return "";
    stringstream res;

    union utf8_char {
        char bs[4];
        unsigned int n;
    };
    utf8_char c;
    for (int i = 0; i < size; ++i) {
        c.n = rand();
        auto n = c.n % 1114112; //max unicode codepoint + 1
        if (0 == n && n <= 127) {
            res << (char)(c.bs[0] & 0b01111111);
        }
        else if (128 <= n && n <= 2047) {
            res << (char)((c.bs[0] | 0b11000000) & 0b11011111);
            res << (char)((c.bs[1] | 0b10000000) & 0b10111111);
        }
        else if (2048 <= n && n <= 65535) {
            res << (char)((c.bs[0] | 0b11100000) & 0b11101111);
            res << (char)((c.bs[1] | 0b10000000) & 0b10111111);
            res << (char)((c.bs[2] | 0b10000000) & 0b10111111);
        }
        else {
            res << (char)((c.bs[0] | 0b11110000) & 0b11110111);
            res << (char)((c.bs[1] | 0b10000000) & 0b10111111);
            res << (char)((c.bs[2] | 0b10000000) & 0b10111111);
            res << (char)((c.bs[3] | 0b10000000) & 0b10111111);
        }
    }
    return res.str();
}

bool rand_bool() {
    return rand();
}

char rand_char() {
    return rand();
}

unsigned char rand_ubyte() {
    return rand();
}

short rand_short() {
    return rand();
}

int rand_int() {
    int res = rand();
    return res;
}

long rand_long() {
    return ((long)rand() << sizeof(int) * 8) | rand();
}

float rand_float() {
    float f1 = (float)rand();
    float f2 = (float)rand();
    if (f2 == 0) ++f2;
    return f1 / f2;
}

double rand_double() {
    double d1 = (double)rand();
    double d2 = (double)rand();
    if (d2 == 0) ++d2;
    return d1 / d2;
}

//date_vals rand_date() {
//    return date_vals(1 + rand() % 10000, 1 + rand() % 12, 1 + rand() % 28);
//}
//
//time_vals rand_time() {
//    return time_vals(rand() % 24, rand() % 60, rand() % 60, rand() % 1000);
//}
//
//datetime_vals rand_datetime() {
//    return datetime_vals(rand_date(), rand_time());
//}

int rand_string(char *dat, int max_size) {
    int len = rand() % max_size;
    if (len == 0) len = 1;
    for (int i = 0; i < len; ++i) {
        dat[i] = ' ' + rand() % ('~' - ' ');
    }
    return len;
}

string rand_string(int size) {
    if (size <= 0) return "";
    stringstream res;
    for (int i = 0; i < size; ++i) {
        res << (char)(' ' + rand() % ('~' - ' '));
    }
    return res.str();
}

string rpad(const string &s, int size, char c) {
    auto pad_size = size - (int)s.size();
    if (pad_size <= 0) return s;
    return str(s, string(pad_size, c));
}

/*
struct network_insert_high_level_fixture {
    string ip = "localhost";
    int port = 5000;
    bool ssl=false;
    network_insert_high_level_fixture() {
        // auto argc = boost::unit_test::framework::master_test_suite().argc;
        // auto argv = boost::unit_test::framework::master_test_suite().argv;
        auto argc = 5;
        std::vector<std::string> argv = {"127.0.0.1", "5000", "true", "bla"};
        unsigned int seed = time(nullptr);
        if (argc > 1) ip = string(argv[1]);
        if (argc > 2) port = stoi(argv[2]);
        if (argc > 3) ssl = (bool)stoi(argv[3]);
        if (argc > 4) seed = stol(argv[4]);
        printf("seed is %ud\n", seed);
        srand(seed);
        sqc.connect(ip, port, ssl, "sqream", "sqream", "master");
        try {
            run_direct_query(&sqc, str("drop database ", db_name));
            sqc.finish_query();
        }
        catch (...) {}
        run_direct_query(&sqc, str("create database ", db_name));
        sqc.connect(ip, port, ssl, "sqream", "sqream", db_name);
    }
};
static unique_ptr<network_insert_high_level_fixture> f = nullptr;
struct init_network_insert_high_level_fixture {
    init_network_insert_high_level_fixture() {
        if (f == nullptr) f.reset(new network_insert_high_level_fixture);
        if(sqc.state_ != 15) sqc.sqc_->close_statement();
    }
    ~init_network_insert_high_level_fixture()
    {
    }
};
//*/


// Tests Start
// -----------

bool test_pass = false, tests_pass = false;

bool setup_tests (string ip = "127.0.0.1", int port = 5001, bool ssl = true){

    auto argc = 5;
    unsigned int seed = time(nullptr);
    srand(seed);
    // sqc.connect(ip, port, ssl, "sqream", "sqream", "master");
    /*
    try {
        printf ("b4 run_direct_query\n");
        run_direct_query(&sqc, str("drop database ", db_name));
        printf ("after run_direct_query\n");
        sqc.finish_query();
    }
    catch (...) {}
    //*/
    // printf ("b4 run_direct_query\n");

    // run_direct_query(&sqc, str("create database ", db_name));
    sqc.connect(ip, port, ssl, "sqream", "sqream", db_name);

    return test_pass;
}

// BOOST_FIXTURE_TEST_SUITE(runtime_test, init_network_insert_high_level_fixture)

string current_test = "Runtime test suite";


/*
bool "explicit_connection_disconnection") {
    std::unique_ptr<sqream::driver> temp(new sqream::driver());
    temp->connect(sqc.sqc_->ipv4_,sqc.sqc_->port_, sqc.sqc_->ssl_, sqc.sqc_->username_, sqc.sqc_->password_, sqc.sqc_->database_, sqc.sqc_->service_);
    new_query_execute(temp.get(), "select 1");
    int row_count = 0;
    bool p;
    while ((p=temp->next_query_row())) {
        assert(temp->get_int(0));
        ++row_count;
    }
    assert(row_count == 1);
    temp->finish_query();
    temp.reset(nullptr);
}
//*/

bool simple() {

    puts("\n```    start simple test");
    run_direct_query(&sqc, "create or replace table t(x int not null)");
    puts("\n```    simple test after create statement");
    new_query_execute(&sqc, "insert into t values(?)");
    puts("\n```    simple test after insert statement");
    int val = rand();
    sqc.set_int(0, val);
    sqc.next_query_row();
    sqc.finish_query();
    ping
    puts("```    simple test after create");

    new_query_execute(&sqc, "select * from t");
    int row_count = 0;
    bool p;
    while ((p=sqc.next_query_row())) {
        assert(sqc.get_int(0) == val);
        ++row_count;
    }
    assert(row_count == 1);
    sqc.finish_query();
    puts("```    end simple test");

    return test_pass;
}


bool simple_tiny_chunk() {

    run_direct_query(&sqc, "create or replace table t (x int not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    int nrows = 1;

    unsigned int seed = rand();
    srand(seed);
    for (int i = 0; i < nrows; ++i) {
        sqc.set_int(0, rand());
        sqc.next_query_row();
    }
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    int row_count = 0;
    srand(seed);
    while (sqc.next_query_row()) {
        assert(sqc.get_int(0) == rand());
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();

    return test_pass;
}


bool mix_nvarchar_null() {

    run_direct_query(&sqc, "create or replace table t (x nvarchar(40))");
    new_query_execute(&sqc, "insert into t values (?)");

    sqc.set_nvarchar(0,"this is a dot.");
    sqc.next_query_row();
    sqc.set_nvarchar(0,"this might be a plus+");
    sqc.next_query_row();
    sqc.set_null(0);
    sqc.next_query_row();
    sqc.set_nvarchar(0,"this definitely is a minus-");
    sqc.next_query_row();
    sqc.set_nvarchar(0,"this looks like a zero0");
    sqc.next_query_row();
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    sqc.next_query_row();
    assert(sqc.get_nvarchar(0) == "this is a dot.");
    sqc.next_query_row();
    assert(sqc.get_nvarchar(0) == "this might be a plus+");
    sqc.next_query_row();
    assert(sqc.is_null(0) == true);
    sqc.next_query_row();
    assert(sqc.get_nvarchar(0) == "this definitely is a minus-");
    sqc.next_query_row();
    assert(sqc.get_nvarchar(0) == "this looks like a zero0");
    sqc.next_query_row();
    sqc.finish_query();

    return test_pass;
}


bool multiple_nvarchar_column() {

    run_direct_query(&sqc, "create or replace table \"public\".\"customers\" (\"a\" bigint null identity(1,1) assert ('CS \"default\"'),\"id\" int not null assert ('CS \"default\"'),\"fname\" varchar(20) not null assert ('CS \"default\"'),\"lname\" nvarchar(20) null assert ('CS \"default\"'))");
    run_direct_query(&sqc, "create or replace table \"public\".\"ncustomers_sales\" (\"id\" int not null assert ('CS \"default\"'),\"name_var\" varchar(20) not null assert ('CS \"default\"'),\"name_nvar\" nvarchar(20) not null assert ('CS \"default\"'),\"sales\" double not null assert ('CS \"default\"'))");
    new_query_execute(&sqc, "insert into customers values (?,?,?,?)");
    sqc.set_long(0, 6);
    sqc.set_int(1, 1);
    sqc.set_varchar(2, "Lea");
    sqc.set_nvarchar(3, "Huntington");
    sqc.next_query_row();
    
    sqc.set_long(0, 7);
    sqc.set_int(1, 2);
    sqc.set_varchar(2, "George");
    sqc.set_nvarchar(3, "Li");
    sqc.next_query_row();
    
    sqc.set_long(0, 8);
    sqc.set_int(1, 3);
    sqc.set_varchar(2, "Leo");
    sqc.set_nvarchar(3, "Bridge");
    sqc.next_query_row();
    
    sqc.set_long(0, 9);
    sqc.set_int(1, 4);
    sqc.set_varchar(2, "Tao");
    sqc.set_nvarchar(3, "Chung");
    sqc.next_query_row();
    sqc.finish_query();
    
    new_query_execute(&sqc, "insert into ncustomers_sales values (?,?,?,?)");
    sqc.set_int(0, 1);
    sqc.set_varchar(1, "AAA");
    sqc.set_nvarchar(2, "AAA");
    sqc.set_double(3, 100.0);
    sqc.next_query_row();
    
    sqc.set_int(0, 2);
    sqc.set_varchar(1, "AAA");
    sqc.set_nvarchar(2, "AAA");
    sqc.set_double(3, 75.0);
    sqc.next_query_row();
    
    sqc.set_int(0, 3);
    sqc.set_varchar(1, "BBB");
    sqc.set_nvarchar(2, "BBB");
    sqc.set_double(3, 25.0);
    sqc.next_query_row();
    
    sqc.set_int(0, 4);
    sqc.set_varchar(1, "BBB");
    sqc.set_nvarchar(2, "BBB");
    sqc.set_double(3, 100.0);
    sqc.next_query_row();
    sqc.finish_query();

    new_query_execute(&sqc, "select top 1000 1 as \"number_of_records\",   \"customers\".\"a\" as \"a\",   \"customers\".\"fname\" as \"fname\",   \"customers\".\"id\" as \"id\",   \"ncustomers_sales\".\"id\" as \"id__ncustomers_sales_\",   \"customers\".\"lname\" as \"lname\",   \"ncustomers_sales\".\"name_nvar\" as \"name_nvar\",   \"ncustomers_sales\".\"name_var\" as \"name_var\",   \"ncustomers_sales\".\"sales\" as \"sales\" from \"public\".\"customers\" \"customers\"   inner join \"public\".\"ncustomers_sales\" \"ncustomers_sales\" on (\"customers\".\"id\" = \"ncustomers_sales\".\"id\")");
    sqc.next_query_row();
        
    assert(sqc.get_int(0) == 1);
    assert(sqc.get_long(1) == 6L);
    assert(sqc.get_varchar(2) == "Lea                 ");
    assert(sqc.get_int(3) == 1);
    assert(sqc.get_int(4) == 1);
    assert(sqc.get_nvarchar(5) == "Huntington");
    assert(sqc.get_nvarchar(6) == "AAA");
    assert(sqc.get_varchar(7) == "AAA                 ");
    assert(sqc.get_double(8) == 100.0);
    sqc.next_query_row();
    
    assert(sqc.get_int(0) == 1);
    assert(sqc.get_long(1) == 7L);
    assert(sqc.get_varchar(2) == "George              ");
    assert(sqc.get_int(3) == 2);
    assert(sqc.get_int(4) == 2);
    assert(sqc.get_nvarchar(5) == "Li");
    assert(sqc.get_nvarchar(6) == "AAA");
    assert(sqc.get_varchar(7) == "AAA                 ");
    assert(sqc.get_double(8) == 75.0);
    sqc.next_query_row();
    
    assert(sqc.get_int(0) == 1);
    assert(sqc.get_long(1) == 8L);
    assert(sqc.get_varchar(2) == "Leo                 ");
    assert(sqc.get_int(3) == 3);
    assert(sqc.get_int(4) == 3);
    assert(sqc.get_nvarchar(5) == "Bridge");
    assert(sqc.get_nvarchar(6) == "BBB");
    assert(sqc.get_varchar(7) == "BBB                 ");
    assert(sqc.get_double(8) == 25.0);
    sqc.next_query_row();
    
    assert(sqc.get_int(0) == 1);
    assert(sqc.get_long(1) == 9L);
    assert(sqc.get_varchar(2) == "Tao                 ");
    assert(sqc.get_int(3) == 4);
    assert(sqc.get_int(4) == 4);
    assert(sqc.get_nvarchar(5) == "Chung");
    assert(sqc.get_nvarchar(6) == "BBB");
    assert(sqc.get_varchar(7) == "BBB                 ");
    assert(sqc.get_double(8) == 100.0);
    sqc.next_query_row();
        
    sqc.finish_query();

    return test_pass;
}

bool simple_tiny_chunk2() {

    run_direct_query(&sqc, "create or replace table t (x int not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    int nrows = 2;

    for (int i = 0; i < nrows; ++i) {
        sqc.set_int(0, 1234);
        sqc.next_query_row();
    }
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    int row_count = 0;
    while (sqc.next_query_row()) {
        assert(sqc.get_int(0) == 1234);
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();

    return test_pass;
}


bool empty_string_chunk() {
    run_direct_query(&sqc, "create or replace table t (x nvarchar(10) not null)");
    new_query_execute(&sqc, "insert into t values (?)");

    string val = ""; // empty string
    sqc.set_nvarchar(0, val);
    sqc.next_query_row();
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    while (sqc.next_query_row()) {
        assert(sqc.get_nvarchar(0).empty() == true);
    }
    sqc.finish_query();

    return test_pass;
}


/*START POSITIVE TESTING*/
bool int_insert_positive_test() {
    run_direct_query(&sqc, "create or replace table t (x int not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    auto nrows = 3;
    int vals[] {INT_MAX, INT_MIN, 0};
    sqc.set_int(0, vals[0]);
    sqc.next_query_row();
    sqc.set_int(0, vals[1]);
    sqc.next_query_row();
    sqc.set_int(0, vals[2]);
    sqc.next_query_row();
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    int row_count = 0;
    while (sqc.next_query_row()) {
        assert(sqc.get_int(0) == vals[row_count]);
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();

    return test_pass;
}


bool float_insert_positive_test() {
    run_direct_query(&sqc, "create or replace table t (x double not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    auto nrows = 7;
    double vals[]
    {
         std::numeric_limits<double>::max()
        ,-std::numeric_limits<double>::max()
        ,std::numeric_limits<double>::min()
        ,-std::numeric_limits<double>::min()
        ,std::numeric_limits<double>::infinity()
        ,-std::numeric_limits<double>::infinity()
        ,std::numeric_limits<double>::quiet_NaN()
    };

    sqc.set_double(0, vals[0]);
    sqc.next_query_row();
    sqc.set_double(0, vals[1]);
    sqc.next_query_row();
    sqc.set_double(0, vals[2]);
    sqc.next_query_row();
    sqc.set_double(0, vals[3]);
    sqc.next_query_row();
    sqc.set_double(0, vals[4]);
    sqc.next_query_row();
    sqc.set_double(0, vals[5]);
    sqc.next_query_row();
    sqc.set_double(0, vals[6]);
    sqc.next_query_row();
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    sqc.next_query_row();
    assert(sqc.get_double(0) == vals[0]);
    sqc.next_query_row();
    assert(sqc.get_double(0) == vals[1]);
    sqc.next_query_row();
    assert(sqc.get_double(0) == vals[2]);
    sqc.next_query_row();
    assert(sqc.get_double(0) == vals[3]);
    sqc.next_query_row();
    assert(std::isinf(sqc.get_double(0)) == true);
    sqc.next_query_row();
    assert(std::isinf(sqc.get_double(0)) == true);
    sqc.next_query_row();
    assert(std::isnan(sqc.get_double(0)) == true);
    sqc.finish_query();

    return test_pass;
}


bool bool_insert_positive_test() {

    run_direct_query(&sqc, "create or replace table t (x bool not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    auto nrows = 2;
    bool vals[] {true, false};
    sqc.set_bool(0, vals[0]);
    sqc.next_query_row();
    sqc.set_bool(0, vals[1]);
    sqc.next_query_row();
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    int row_count = 0;
    while (sqc.next_query_row()) {
        assert(sqc.get_bool(0) == vals[row_count]);
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();

    return test_pass;
}


bool date_insert_positive_test() {

    run_direct_query(&sqc, "create or replace table t (x date not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    auto nrows = 27;
    sqream::date_t vals[]
    {
         sqream::make_date(sqream::date(0, 1, 1)), sqream::make_date(sqream::date(9999, 12, 31)) //date range min/max
        ,sqream::make_date(sqream::date(1111, 1,1)),sqream::make_date(sqream::date(1111, 1, 31)) //######## Testing all the ranges for every month (1 to 30/31 days precisely), (PS: 1111 is not a leap year)
        ,sqream::make_date(sqream::date(1111, 2,1)),sqream::make_date(sqream::date(1111, 2, 28))
        ,sqream::make_date(sqream::date(1111, 3,1)),sqream::make_date(sqream::date(1111, 3, 31))
        ,sqream::make_date(sqream::date(1111, 4,1)),sqream::make_date(sqream::date(1111, 4, 30))
        ,sqream::make_date(sqream::date(1111, 5,1)),sqream::make_date(sqream::date(1111, 5, 31))
        ,sqream::make_date(sqream::date(1111, 6,1)),sqream::make_date(sqream::date(1111, 6, 30))
        ,sqream::make_date(sqream::date(1111, 7,1)),sqream::make_date(sqream::date(1111, 7, 31))
        ,sqream::make_date(sqream::date(1111, 8,1)),sqream::make_date(sqream::date(1111, 8, 31))
        ,sqream::make_date(sqream::date(1111, 9,1)),sqream::make_date(sqream::date(1111, 9, 30))
        ,sqream::make_date(sqream::date(1111, 10,1)),sqream::make_date(sqream::date(1111, 10, 31))
        ,sqream::make_date(sqream::date(1111, 11,1)),sqream::make_date(sqream::date(1111, 11, 30))
        ,sqream::make_date(sqream::date(1111, 12,1)),sqream::make_date(sqream::date(1111, 12, 31)) //#######
        ,sqream::make_date(sqream::date(2016, 2, 29)) //leap year !
    };

    for(int i = 0; i < nrows; i++)
    {
        sqc.set_date(0, sqream::date(vals[i].year, vals[i].month, vals[i].day));
        sqc.next_query_row();
    }
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    int row_count = 0;
    while (sqc.next_query_row()) {
        assert(comp_date_t(sqream::make_date(sqc.get_date(0)), vals[row_count]) == true);
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();

    return test_pass;
}


bool datetime_insert_positive_test() {

    run_direct_query(&sqc, "create or replace table t (x datetime not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    auto nrows = 29;
    sqream::datetime_t vals[]
        {
            sqream::make_datetime(sqream::datetime(0, 1, 1, 0, 0, 0, 0)), sqream::make_datetime(sqream::datetime(9999, 12, 31, 0, 0, 0, 0)) //date range min/max
            ,sqream::make_datetime(sqream::datetime(1111, 1,1, 0, 0, 0, 0)),sqream::make_datetime(sqream::datetime(1111, 1, 31, 0, 0, 0, 0)) //######## Testing all the ranges for every month (1 to 30/31 days precisely), (PS: 1111 is not a leap year)
            ,sqream::make_datetime(sqream::datetime(1111, 2,1, 0, 0, 0, 0)),sqream::make_datetime(sqream::datetime(1111, 2, 28, 0, 0, 0, 0))
            ,sqream::make_datetime(sqream::datetime(1111, 3,1, 0, 0, 0, 0)),sqream::make_datetime(sqream::datetime(1111, 3, 31, 0, 0, 0, 0))
            ,sqream::make_datetime(sqream::datetime(1111, 4,1, 0, 0, 0, 0)),sqream::make_datetime(sqream::datetime(1111, 4, 30, 0, 0, 0, 0))
            ,sqream::make_datetime(sqream::datetime(1111, 5,1, 0, 0, 0, 0)),sqream::make_datetime(sqream::datetime(1111, 5, 31, 0, 0, 0, 0))
            ,sqream::make_datetime(sqream::datetime(1111, 6,1, 0, 0, 0, 0)),sqream::make_datetime(sqream::datetime(1111, 6, 30, 0, 0, 0, 0))
            ,sqream::make_datetime(sqream::datetime(1111, 7,1, 0, 0, 0, 0)),sqream::make_datetime(sqream::datetime(1111, 7, 31, 0, 0, 0, 0))
            ,sqream::make_datetime(sqream::datetime(1111, 8,1, 0, 0, 0, 0)),sqream::make_datetime(sqream::datetime(1111, 8, 31, 0, 0, 0, 0))
            ,sqream::make_datetime(sqream::datetime(1111, 9,1, 0, 0, 0, 0)),sqream::make_datetime(sqream::datetime(1111, 9, 30, 0, 0, 0, 0))
            ,sqream::make_datetime(sqream::datetime(1111, 10,1, 0, 0, 0, 0)),sqream::make_datetime(sqream::datetime(1111, 10, 31, 0, 0, 0, 0))
            ,sqream::make_datetime(sqream::datetime(1111, 11,1, 0, 0, 0, 0)),sqream::make_datetime(sqream::datetime(1111, 11, 30, 0, 0, 0, 0))
            ,sqream::make_datetime(sqream::datetime(1111, 12,1, 0, 0, 0, 0)),sqream::make_datetime(sqream::datetime(1111, 12, 31, 0, 0, 0, 0)) //#######
            ,sqream::make_datetime(sqream::datetime(0, 1, 1, 0, 0, 0, 0)), sqream::make_datetime(sqream::datetime(9999, 12, 31, 23, 59, 59, 999)) //time range min/max
            ,sqream::make_datetime(sqream::datetime(2016, 2, 29, 0, 0, 0, 0)) //leap year !
        };

    for(int i = 0; i < nrows; i++)
    {
        sqc.set_datetime(0, sqream::datetime(vals[i].year, vals[i].month, vals[i].day, vals[i].hour, vals[i].minute, vals[i].second, vals[i].millisecond));
        sqc.next_query_row();
    }
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    int row_count = 0;
    while (sqc.next_query_row()) {
        assert(comp_datetime_t(sqream::make_datetime(sqc.get_datetime(0)), vals[row_count]) == true);
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();

    return test_pass;
}

/*
bool time_insert_positive_test() {
    run_direct_query(&sqc, "create or replace table t (x datetime not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    auto nrows = 2;
    date_vals date = sqream::make_date(sqream::date(1111,1,1);
    time_vals time[nrows] { time_vals(0, 0, 0, 0), time_vals(23, 59, 59, 999) };
    datetime_vals vals[] { datetime_vals(date, time[0]), datetime_vals(date, time[1]) };
    sqc.set_datetime(0, vals[0]);
    sqc.next_query_row();
    sqc.set_datetime(0, vals[1]);
    sqc.next_query_row();
    sqc.finish_query();
    new_query_execute(&sqc, "select * from t");
    int row_count = 0;
    while (sqc.next_query_row()) {
        assert(sqc.get_datetime(0) == vals[row_count]);
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();
}  //*/


bool varchar_insert_positive_test() {

    run_direct_query(&sqc, "create or replace table t (x varchar(10) not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    auto nrows = 1;
    string val = "Default   "; //Is 10 characters
    sqc.set_varchar(0, val);
    sqc.next_query_row();
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    int row_count = 0;
    while (sqc.next_query_row()) {
        assert(sqc.get_varchar(0) == val);
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();

    run_direct_query(&sqc, "truncate table t");
    new_query_execute(&sqc, "insert into t values (?)");
    val = "Default"; //Is 7 characters - will be padded to 10 automatically
    sqc.set_varchar(0, val);
    sqc.next_query_row();
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    row_count = 0;
    while (sqc.next_query_row()) {
        assert(sqc.get_varchar(0) == rpad(val, 10, ' '));
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();

    run_direct_query(&sqc, "truncate table t");
    new_query_execute(&sqc, "insert into t values (?)");
    val = ""; // empty string
    sqc.set_varchar(0, val);
    sqc.next_query_row();
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    row_count = 0;
    while (sqc.next_query_row()) {
        assert(sqc.get_varchar(0) == rpad(val, 10, ' '));
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();

    run_direct_query(&sqc, "truncate table t");
    new_query_execute(&sqc, "insert into t values (?)");
    val = "12435"; // string of size 5 in varchar(10) col
    sqc.set_varchar(0, val);
    sqc.next_query_row();
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    row_count = 0;
    while (sqc.next_query_row()) {
        assert(sqc.get_varchar(0) == rpad(val, 10, ' '));
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();

    return true;
}

bool nvarchar_insert_positive_test() {

    run_direct_query(&sqc, "create or replace table t (x nvarchar(10) not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    auto nrows = 3;
    string vals[] {"Default", "four", "בדיקה"}; //first is 7 characters, second is 4, third is 5
    for (auto &v : vals) {
        sqc.set_nvarchar(0, v);
        sqc.next_query_row();
    }
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    int row_count = 0;
    while (sqc.next_query_row()) {
        assert(sqc.get_nvarchar(0) == vals[row_count]);
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();

    nrows = 1;

    run_direct_query(&sqc, "truncate table t");
    new_query_execute(&sqc, "insert into t values (?)");
    string empty_val = ""; // empty string
    sqc.set_nvarchar(0, empty_val);
    sqc.next_query_row();
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    row_count = 0;
    while (sqc.next_query_row()) {
        assert(sqc.get_nvarchar(0).empty() == true);
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();

    run_direct_query(&sqc, "truncate table t");
    new_query_execute(&sqc, "insert into t values (?)");
    string normal_val = "12435"; // string of size 5 in nvarchar(10) col - will not be padded to 10 automatically
    sqc.set_nvarchar(0, normal_val);
    sqc.next_query_row();
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    row_count = 0;
    while (sqc.next_query_row()) {
        assert(sqc.get_nvarchar(0) == normal_val);
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();

    return test_pass;
}


bool nvarchar_insert_positive_test_big() {

    run_direct_query(&sqc, "create or replace table t (x nvarchar(10) not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    auto nrows = 2 * 1024 * 1024;
    auto seed = rand();
    srand(seed);
    for (int i = 0; i < nrows; ++i) {
        sqc.set_nvarchar(0, rand_utf8_string(10));
        sqc.next_query_row();
    }
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    int row_count = 0;
    srand(seed);
    while (sqc.next_query_row()) {
        assert(sqc.get_nvarchar(0) == rand_utf8_string(10));
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();

    return test_pass;
}


/*
bool "get_chunk_size_positive_test) {
    run_direct_query(&sqc, "create or replace table t (x int not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    auto nrows = 5;
    int vals[] {1, 2, 3, 4, 5};
    for(int i = 0; i < nrows; i++)
    {
        sqc.set_int(0, vals[i]);
        sqc.next_query_row();
    }
    sqc.finish_query();
    new_query_execute(&sqc, "select * from t");
    int row_count = 0;
    while (sqc.next_query_row()) {
        ++row_count;
    }
    assert(row_count == nrows);
    assert(get_chunk_size(s.get()) == nrows);
    sqc.finish_query();
}*/


bool get_columns_info_positive_test() {

    run_direct_query(&sqc, "create or replace table t (x_v varchar(10) not null, x_i int not null, x_v_n varchar(10) null, x_i_n int null)");

    string s_val = "default";
    int i_val = 1;

    new_query_execute(&sqc, "insert into t values (?,?,?,?)");
    sqc.set_varchar(0, s_val);
    sqc.set_int(1, i_val);
    sqc.set_null(2);
    sqc.set_null(3);
    sqc.next_query_row();
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    auto columns = get_metadata(&sqc);

    //We retrieve the column infos (which are send to metadata)
    //And we control that every column has the right 4 informations (name, type, nul, tvc)
    assert(columns[0].name == "x_v");
    assert(columns[0].type == "ftVarchar");
    assert(columns[0].nullable == false);
    assert(columns[0].is_true_varchar == false);

    assert(columns[1].name == "x_i");
    assert(columns[1].type == "ftInt");
    assert(columns[1].nullable == false);
    assert(columns[1].is_true_varchar == false);

    assert(columns[2].name == "x_v_n");
    assert(columns[2].type == "ftVarchar");
    assert(columns[2].nullable == true);
    assert(columns[2].is_true_varchar == false);

    assert(columns[3].name == "x_i_n");
    assert(columns[3].type == "ftInt");
    assert(columns[3].nullable == true);
    assert(columns[3].is_true_varchar == false);
    sqc.finish_query();

    return test_pass;
}

// TEST DEFAULT
bool int_insert_default_positive_test() {

    auto nrows = 3;
    int vals[] {INT_MAX, INT_MIN, 0};

    run_direct_query(&sqc, "create or replace table t (x int not null, y int default 7)");
    new_query_execute(&sqc, "insert into t(x) values (?)");
    sqc.set_int(0, vals[0]);
    sqc.next_query_row();
    sqc.set_int(0, vals[1]);
    sqc.next_query_row();
    sqc.set_int(0, vals[2]);
    sqc.next_query_row();
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    int row_count = 0;
    while (sqc.next_query_row()) {
        assert(sqc.get_int(0) == vals[row_count]);
        assert(sqc.get_int(1) == 7);
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();

    return test_pass;
}

// TEST DEFAULT VARCHAR
bool varchar_insert_default_positive_test() {

    int nrows = 3;
    int vals[] {INT_MAX, INT_MIN, 0};
    run_direct_query(&sqc, "create or replace table t (x int not null, y varchar(10) default 'hello')");
    new_query_execute(&sqc, "insert into t(x) values (?)");
    for (int i = 0; i < nrows; ++i) {
        sqc.set_int(0, vals[i]);
        sqc.next_query_row();
    }
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    int row_count = 0;
    while (sqc.next_query_row()) {
        assert(sqc.get_int(0) == vals[row_count]);
        assert(sqc.get_varchar(1) == "hello     ");
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();

    return test_pass;
}

// TEST IDENTITY
bool int_insert_identity_positive_test() {

    auto nrows = 3;
    int vals[] {INT_MAX, INT_MIN, 0};
    int identity = 17;

    run_direct_query(&sqc, "create or replace table t (x int not null, y int identity(17, 1))");
    new_query_execute(&sqc, "insert into t(x) values (?)");
    sqc.set_int(0, vals[0]);
    sqc.next_query_row();
    sqc.set_int(0, vals[1]);
    sqc.next_query_row();
    sqc.set_int(0, vals[2]);
    sqc.next_query_row();
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    int row_count = 0;
    while (sqc.next_query_row()) {
        assert(sqc.get_int(0) == vals[row_count]);
        assert(sqc.get_int(1) == identity++);
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();

    return test_pass;
}


bool driver_internal_struct_coverage() {
    //Filling Coverage missing function calls for simple structures

    //sqream::column
    const sqream::column temp = {"my_column", false, false, "my_type", 4, 0};
    sqream::column copy = {"my_column2", false, false, "my_type2", 4, 0};
    copy = temp;
    auto moved = std::move(copy);

    return test_pass;
}

/*
bool retrieve_stmt_id() {
    std::unique_ptr<sqream::driver> temp(new sqream::driver());
    REQUIRE_THROWS_AS(retrieve_statement_id(temp.get()), std::string);
    temp->connect(sqc.sqc_->ipv4_,sqc.sqc_->port_, sqc.sqc_->ssl_, sqc.sqc_->username_, sqc.sqc_->password_, sqc.sqc_->database_, sqc.sqc_->service_);
    new_query_execute(temp.get(), "select 1");
    int row_count = 0;
    while (temp->next_query_row()) {
        assert(temp->get_int(0));
        ++row_count;
    }
    retrieve_statement_id(temp.get());
    assert(row_count == 1);
    temp->finish_query();
    return test_pass;
}
bool retrieve_stmt_type() {
    std::unique_ptr<sqream::driver> temp(new sqream::driver());
    REQUIRE_THROWS_AS(retrieve_statement_type(temp.get()), std::string);
    temp->connect(sqc.sqc_->ipv4_,sqc.sqc_->port_, sqc.sqc_->ssl_, sqc.sqc_->username_, sqc.sqc_->password_, sqc.sqc_->database_, sqc.sqc_->service_);
    assert(retrieve_statement_type(temp.get()) == con::CONSTS::unset);
    run_direct_query(temp.get(), "create or replace table retrieve_types(x int not null)");
    assert(retrieve_statement_type(temp.get()) == con::CONSTS::direct);
    new_query_execute(temp.get(), "insert into retrieve_types values(?)");
    assert(retrieve_statement_type(temp.get()) == con::CONSTS::insert);
    temp->set_int(0, 666);
    temp->next_query_row();
    temp->finish_query();
    new_query_execute(temp.get(), "select * from retrieve_types");
    assert(retrieve_statement_type(temp.get()) == con::CONSTS::select);
    while (temp->next_query_row()) {}
    temp->finish_query();
    return test_pass;
}
//*/

/*END OF POSITIVE TESTING*/

// NEGATIVE TESTING FOR SETTERS

/*
//Here we need to figure out the way of doing eli's create or replace table function
bool negative_testing_types_not_null() {
    run_direct_query(&sqc,"create or replace table t (bool0 bool not null,bit1 bit not null,tinyint2 tinyint not null,smallint3 smallint not null,int4 int not null,bigint5 bigint not null,real6 real not null,float7 float not null,date8 date not null,datetime9 datetime not null,varchar_10_10 varchar(10) not null,varchar_100_11 varchar(100) not null)");
    new_query_execute(&sqc, str("insert into t values (?,?,?,?,?,?,?,?,?,?,?,?)"));
    int i = 0;
    REQUIRE_THROWS_AS(sqc.set_null(i++), std::string);
    REQUIRE_THROWS_AS(sqc.set_null(i++), std::string);
    REQUIRE_THROWS_AS(sqc.set_null(i++), std::string);
    REQUIRE_THROWS_AS(sqc.set_null(i++), std::string);
    REQUIRE_THROWS_AS(sqc.set_null(i++), std::string);
    REQUIRE_THROWS_AS(sqc.set_null(i++), std::string);
    REQUIRE_THROWS_AS(sqc.set_null(i++), std::string);
    REQUIRE_THROWS_AS(sqc.set_null(i++), std::string);
    REQUIRE_THROWS_AS(sqc.set_null(i++), std::string);
    REQUIRE_THROWS_AS(sqc.set_null(i++), std::string);
    REQUIRE_THROWS_AS(sqc.set_null(i++), std::string);
    return test_pass;
}
//*/

/*
bool "negative_testing_type_date_not_null) {
    run_direct_query(&sqc, "create or replace table t (x date not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    // wrong year
    REQUIRE_THROWS_AS(sqc.set_date(0, date_vals(-1,1,1)), std::string);
    // wrong month
    REQUIRE_THROWS_AS(sqc.set_date(0, date_vals(1,-1,1)) , std::string);
    // wrong month
    REQUIRE_THROWS_AS(sqc.set_date(0, date_vals(1,13,1)), std::string);
    // wrong day
    REQUIRE_THROWS_AS(sqc.set_date(0, date_vals(1,1,-1)), std::string);
    // wrong day
    REQUIRE_THROWS_AS(sqc.set_date(0, date_vals(1,1,33)), std::string);
}
bool "negative_testing_type_datetime_not_null) {
    run_direct_query(&sqc, "create or replace table t (x datetime not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    REQUIRE_THROWS_AS(
        sqc.set_datetime(0, datetime_vals(date_vals(1111,1,1), time_vals(-1, 1, 1, 1))),
        std::string); // negative hours
    REQUIRE_THROWS_AS(
        sqc.set_datetime(0, datetime_vals(date_vals(1111,1,1), time_vals(25, 1, 1, 1))),
        std::string); // illegal hour
    REQUIRE_THROWS_AS(
        sqc.set_datetime(0, datetime_vals(date_vals(1111,1,1), time_vals(1, -1, 1, 1))),
        sqream_exception); // negative min
    REQUIRE_THROWS_AS(
        sqc.set_datetime(0, datetime_vals(date_vals(1111,1,1), time_vals(1, 60, 1, 1))),
        sqream_exception); // illegal min
    REQUIRE_THROWS_AS(
        sqc.set_datetime(0, datetime_vals(date_vals(1111,1,1), time_vals(1, 1, -1, 1))),
        sqream_exception); // negative sec
    REQUIRE_THROWS_AS(
        sqc.set_datetime(0, datetime_vals(date_vals(1111,1,1), time_vals(1, 1, 60, 1))),
        sqream_exception); // illegal sec
    REQUIRE_THROWS_AS(
        sqc.set_datetime(0, datetime_vals(date_vals(1111,1,1), time_vals(1, 1, 1, -1))),
        sqream_exception); // negative msec
    REQUIRE_THROWS_AS(
        sqc.set_datetime(0, datetime_vals(date_vals(1111,1,1), time_vals(1, 1, 1, 1000))),
        sqream_exception); // illegal msecs
}*/


/*
bool negative_testing_type_varchar_not_null() {
    run_direct_query(&sqc, "create or replace table t (x varchar(10) not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    string val = "12345678901"; // overflow - 11 chars
    REQUIRE_THROWS_AS(sqc.set_varchar(0, val), std::string);
    REQUIRE_THROWS_AS(sqc.set_null(0), std::string);
    return test_pass;
}
bool negative_testing_type_nvarchar_not_null() {
    run_direct_query(&sqc, "create or replace table t (x nvarchar(10) not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    string val = "12345678901"; // overflow - 11 chars
    sqc.set_nvarchar(0, val);
    sqc.next_query_row(0);
    REQUIRE_THROWS_AS(sqc.finish_query(), std::string);
    new_query_execute(&sqc, "insert into t values (?)");
    val = "אבגדהוזחטיכ"; // overflow - 11 chars
    sqc.set_nvarchar(0, val);
    sqc.next_query_row(0);
    REQUIRE_THROWS_AS(sqc.finish_query(), std::string);
    new_query_execute(&sqc, "insert into t values (?)");
    REQUIRE_THROWS_AS(sqc.set_null(0), std::string);
    return test_pass;
}
//*/

/* END OF NEGATIVE TESTING FOR SETTERS*/

bool simple_bulk() {

    run_direct_query(&sqc, "create or replace table t (x int not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    int nrows = 1024 * 1024;

    unsigned int seed = rand();
    srand(seed);
    for (int i = 0; i < nrows; ++i) {
        sqc.set_int(0, rand());
        sqc.next_query_row();
    }
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    int row_count = 0;
    srand(seed);
    while (sqc.next_query_row(1<<26)) {
        assert(sqc.get_int(0) == rand());
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();

    return test_pass;
}


bool all_types() {

    run_direct_query(&sqc,"create or replace table t (bool0 bool not null,bit1 bit not null,tinyint2 tinyint not null,smallint3 smallint not null,int4 int not null,bigint5 bigint not null,real6 real not null,float7 float not null,date8 date not null,datetime9 datetime not null,varchar_10_10 varchar(10) not null,varchar_100_11 varchar(100) not null, nvarchar_20_12 nvarchar(20) not null)");
    new_query_execute(&sqc, "insert into t values (?,?,?,?,?,?,?,?,?,?,?,?,?)");
    int nrows = 1000;
    unsigned int seed = rand();
    srand(seed);
    for (int r = 0; r < nrows; ++r) {
        int i = 0;
        sqc.set_bool(i++, rand_bool());
        sqc.set_bool(i++, rand_bool());
        sqc.set_ubyte(i++, rand_ubyte());
        sqc.set_short(i++, rand_short());
        sqc.set_int(i++, rand());
        sqc.set_long(i++, rand_long());
        sqc.set_float(i++, rand_float());
        sqc.set_double(i++, rand_double());
        sqc.set_date(i++, sqream::date(1991,2,21));
        sqc.set_datetime(i++, sqream::datetime(1991,2,21,10,11,12,666));
        sqc.set_varchar(i++, rand_string(10));
        sqc.set_varchar(i++, rand_string(100));
        sqc.set_nvarchar(i++, rand_string(20));
        sqc.next_query_row();
    }
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");

    int row_count = 0;
    srand(seed);
    while (sqc.next_query_row()) {
        int i = 0;
        assert(sqc.get_bool(i++) == rand_bool());
        assert(sqc.get_bool(i++) == rand_bool());
        assert(sqc.get_ubyte(i++) == rand_ubyte());
        assert(sqc.get_short(i++) == rand_short());
        assert(sqc.get_int(i++) == rand());
        assert(sqc.get_long(i++) == rand_long());
        assert(sqc.get_float(i++) == rand_float());
        assert(sqc.get_double(i++) == rand_double());
        assert(sqc.get_date(i++) == sqream::date(1991,2,21));
        assert(sqc.get_datetime(i++) == sqream::datetime(1991,2,21,10,11,12,666));
        assert(sqc.get_varchar(i++) == rand_string(10));
        assert(sqc.get_varchar(i++) == rand_string(100));
        assert(sqc.get_nvarchar(i++) == rand_string(20));
        ++row_count;
    }
    assert(row_count == nrows);
    sqc.finish_query();

    return test_pass;
}


bool all_types_with_nulls() {

    run_direct_query(&sqc,"create or replace table t (bool0 bool null,bit1 bit null,tinyint2 tinyint null,smallint3 smallint null,int4 int null,bigint5 bigint null,real6 real null,float7 float null,date8 date null,datetime9 datetime null,varchar_10_10 varchar(10) null,varchar_100_11 varchar(100) null, nvarchar_20_12 nvarchar(20) null)");
    new_query_execute(&sqc, "insert into t values (?,?,?,?,?,?,?,?,?,?,?,?,?)");
    int nrows = 500;
    unsigned int seed = rand();
    srand(seed);
    for (int r = 0; r < nrows; ++r) {
        int i = 0;
        sqc.set_bool(i++, rand_bool());
        sqc.set_bool(i++, rand_bool());
        sqc.set_ubyte(i++, rand_ubyte());
        sqc.set_short(i++, rand_short());
        sqc.set_int(i++, rand());
        sqc.set_long(i++, rand_long());
        sqc.set_float(i++, rand_float());
        sqc.set_double(i++, rand_double());
        sqc.set_date(i++, sqream::date(1991,2,21));
        sqc.set_datetime(i++, sqream::datetime(1991,2,21,10,11,12,666));
        sqc.set_varchar(i++, rand_string(10));
        sqc.set_varchar(i++, rand_string(100));
        sqc.set_nvarchar(i++, rand_string(20));
        sqc.next_query_row();
        
        i=0;
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.next_query_row();
    }
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");

    int row_count = 0;
    srand(seed);
    while (sqc.next_query_row()) {
        int i = 0;
        assert(sqc.get_bool(i++) == rand_bool());
        assert(sqc.get_bool(i++) == rand_bool());
        assert(sqc.get_ubyte(i++) == rand_ubyte());
        assert(sqc.get_short(i++) == rand_short());
        assert(sqc.get_int(i++) == rand());
        assert(sqc.get_long(i++) == rand_long());
        assert(sqc.get_float(i++) == rand_float());
        assert(sqc.get_double(i++) == rand_double());
        assert(sqc.get_date(i++) == sqream::date(1991,2,21));
        assert(sqc.get_datetime(i++) == sqream::datetime(1991,2,21,10,11,12,666));
        assert(sqc.get_varchar(i++) == rand_string(10));
        assert(sqc.get_varchar(i++) == rand_string(100));
        assert(sqc.get_nvarchar(i++) == rand_string(20));
        ++row_count;
        if(sqc.next_query_row())
        {
            i=0;
            assert(sqc.is_null(i++) == true);
            assert(sqc.is_null(i++) == true);
            assert(sqc.is_null(i++) == true);
            assert(sqc.is_null(i++) == true);
            assert(sqc.is_null(i++) == true);
            assert(sqc.is_null(i++) == true);
            assert(sqc.is_null(i++) == true);
            assert(sqc.is_null(i++) == true);
            assert(sqc.is_null(i++) == true);
            assert(sqc.is_null(i++) == true);
            assert(sqc.is_null(i++) == true);
            assert(sqc.is_null(i++) == true);
            assert(sqc.is_null(i++) == true);
            ++row_count;
        }
        else
            break;
    }
    assert(row_count == 2*nrows);
    sqc.finish_query();

    return test_pass;
}

/*
bool all_types_with_nulls_named_col() {
    run_direct_query(&sqc,"create or replace table t (bool0 bool null,bit1 bit null,tinyint2 tinyint null,smallint3 smallint null,int4 int null,bigint5 bigint null,real6 real null,float7 float null,date8 date null,datetime9 datetime null,varchar_10_10 varchar(10) null,varchar_100_11 varchar(100) null, nvarchar_20_12 nvarchar(20) null)");
    new_query_execute(&sqc, "insert into t values (?,?,?,?,?,?,?,?,?,?,?,?,?)");
    //Set using column name is still unsupported
    REQUIRE_THROWS_AS(sqc.set_bool("bool0", rand_bool()), std::string);
    REQUIRE_THROWS_AS(sqc.set_bool("bit1", rand_bool()), std::string);
    REQUIRE_THROWS_AS(sqc.set_ubyte("tinyint2", rand_ubyte()), std::string);
    REQUIRE_THROWS_AS(sqc.set_short("smallint3", rand_short()), std::string);
    REQUIRE_THROWS_AS(sqc.set_int("int4", rand()), std::string);
    REQUIRE_THROWS_AS(sqc.set_long("bigint5", rand_long()), std::string);
    REQUIRE_THROWS_AS(sqc.set_float("real6", rand_float()), std::string);
    REQUIRE_THROWS_AS(sqc.set_double("float7", rand_double()), std::string);
    REQUIRE_THROWS_AS(sqc.set_date("date8", sqream::date(1991,2,21)), std::string);
    REQUIRE_THROWS_AS(sqc.set_datetime("datetime9", sqream::datetime(1991,2,21,10,11,12,666)), std::string);
    REQUIRE_THROWS_AS(sqc.set_varchar("varchar_10_10", rand_string(10)), std::string);
    REQUIRE_THROWS_AS(sqc.set_varchar("varchar_100_11", rand_string(100)), std::string);
    REQUIRE_THROWS_AS(sqc.set_nvarchar("nvarchar_20_12", rand_string(20)), std::string);
    REQUIRE_THROWS_AS(sqc.set_null("bool0"), std::string);
    int nrows = 500;
    unsigned int seed = rand();
    srand(seed);
    for (int r = 0; r < nrows; ++r) {
        int i = 0;
        sqc.set_bool(i++, rand_bool());
        sqc.set_bool(i++, rand_bool());
        sqc.set_ubyte(i++, rand_ubyte());
        sqc.set_short(i++, rand_short());
        sqc.set_int(i++, rand());
        sqc.set_long(i++, rand_long());
        sqc.set_float(i++, rand_float());
        sqc.set_double(i++, rand_double());
        sqc.set_date(i++, sqream::date(1991,2,21));
        sqc.set_datetime(i++, sqream::datetime(1991,2,21,10,11,12,666));
        sqc.set_varchar(i++, rand_string(10));
        sqc.set_varchar(i++, rand_string(100));
        sqc.set_nvarchar(i++, rand_string(20));
        sqc.next_query_row();
        i=0;
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.set_null(i++);
        sqc.next_query_row();
    }
    sqc.finish_query();
    new_query_execute(&sqc, "select * from t");
    assert(sqc.is_nullable("bool0") == true);
    assert(sqc.is_nullable("bit1") == true);
    assert(sqc.is_nullable("tinyint2") == true);
    assert(sqc.is_nullable("smallint3") == true);
    assert(sqc.is_nullable("int4") == true);
    assert(sqc.is_nullable("bigint5") == true);
    assert(sqc.is_nullable("real6") == true);
    assert(sqc.is_nullable("float7") == true);
    assert(sqc.is_nullable("date8") == true);
    assert(sqc.is_nullable("datetime9") == true);
    assert(sqc.is_nullable("varchar_10_10") == true);
    assert(sqc.is_nullable("varchar_100_11") == true);
    assert(sqc.is_nullable("nvarchar_20_12") == true);
    int row_count = 0;
    srand(seed);
    while (sqc.next_query_row()) {
        assert(sqc.get_bool("bool0") == rand_bool());
        assert(sqc.get_bool("bit1") == rand_bool());
        assert(sqc.get_ubyte("tinyint2") == rand_ubyte());
        assert(sqc.get_short("smallint3") == rand_short());
        assert(sqc.get_int("int4") == rand());
        assert(sqc.get_long("bigint5") == rand_long());
        assert(sqc.get_float("real6") == rand_float());
        assert(sqc.get_double("float7") == rand_double());
        assert(sqc.get_date("date8") == sqream::date(1991,2,21));
        assert(sqc.get_datetime("datetime9") == sqream::datetime(1991,2,21,10,11,12,666));
        assert(sqc.get_varchar("varchar_10_10") == rand_string(10));
        assert(sqc.get_varchar("varchar_100_11") == rand_string(100));
        assert(sqc.get_nvarchar("nvarchar_20_12") == rand_string(20));
        ++row_count;
        if(sqc.next_query_row())
        {
            assert(sqc.is_null("bool0") == true);
            assert(sqc.is_null("bit1") == true);
            assert(sqc.is_null("tinyint2") == true);
            assert(sqc.is_null("smallint3") == true);
            assert(sqc.is_null("int4") == true);
            assert(sqc.is_null("bigint5") == true);
            assert(sqc.is_null("real6") == true);
            assert(sqc.is_null("float7") == true);
            assert(sqc.is_null("date8") == true);
            assert(sqc.is_null("datetime9") == true);
            assert(sqc.is_null("varchar_10_10") == true);
            assert(sqc.is_null("varchar_100_11") == true);
            assert(sqc.is_null("nvarchar_20_12") == true);
            ++row_count;
        }
        else
            break;
    }
    assert(row_count == 2*nrows);
    sqc.finish_query();
    return test_pass;
}
//*/

//////// test the insert sequence operation
/*
    negative testing
        set_row / get_row: an error should be thrown in the case of running set_row
            without an insert query
        get_row: in the case of running get_row when there is nothing to read an error should be thrown
            EyalW - can not be done since  get / set row usese the same funcation next_query_row() tha tis changed
                 according to type (insert / select)
        call set function in the middle of select statement
*/

// next_row:in the case of running next row with partial sets on the columns an error should be thrown
// set does not match column count before next row
bool negative_testing_next_row_column_count() {

    run_direct_query(&sqc, "create or replace table t (x int not null, y int not null)");
    new_query_execute(&sqc, "insert into t values (?,?)");
    sqc.set_int(0, 5);
    try
    {
        sqc.next_query_row();
        assert(false);
    }
    catch(std::string &se)
    {
        assert(se.find(ERR_SQMC) != string::npos);
    }

    return test_pass;
}


//new_query_execute not matching table, table t (x int not null), "insert into t values (?,?)");
bool negative_testing_prepare_column_count(){

    run_direct_query(&sqc, "create or replace table t (x int not null, y int not null)");
    try
    {
        new_query_execute(&sqc, "insert into t values (?)");
        assert(false);
    }
    catch(std::string &se)
    {
        assert(se.find(ERR_SQMC) != string::npos);
    }

    return test_pass;
}

// set wrong INDEX
bool negative_testing_set_wrong_index() {

    run_direct_query(&sqc, "create or replace table t (x int not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    sqc.set_int(0, 1);
    try
    {
        sqc.set_int(3, 3);
        assert(false);
    }
    catch(std::string &se)
    {
        assert(se.find(ERR_SQMC) != string::npos);
    }

    return test_pass;
}

// **** EyalW can not work for now -> we don't have names in the type in
// set wrong NAME
//bool "negative_testing_set_wrong_name)
//{
//    run_direct_query(&sqc, "create or replace table t (x int not null)");
//    new_query_execute(&sqc, "insert into t values (?)");
//    try
//    {
//        sqc.set_int("x", 1);
//    }
//    catch(std::string &se)
//    {
//        std::cout << se.c_str() << std::endl;
//    }
//    try
//    {
//        sqc.set_int("z", 3);
//        assert(false);
//    }
//    catch(std::string &se)
//    {
//        std::cout << se.c_str() << std::endl;
//        // looking for ERR_INVALID_COLUMN_NAME
//        assert(se.find(ERR_INVALID_COLUMN_NAME) != string::npos);
//    }
//}

// get wrong INDEX
bool negative_testing_get_wrong_index(){

    run_direct_query(&sqc, "create or replace table t (x int not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    sqc.set_int(0, 0);
    sqc.next_query_row();
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    sqc.next_query_row();
    try
    {
        sqc.get_int(1);
        assert(false);
    }
    catch(std::string &se)
    {
        // looking for ERR_COLUMN_INDEX_OUT_OF_RANGE
        assert(se.find(ERR_SQMC) != string::npos);
    }

    return test_pass;
}


// get wrong NAME
bool negative_testing_get_wrong_name(){

    run_direct_query(&sqc, "create or replace table t (x int not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    int val = rand();
    sqc.set_int(0, val);
    sqc.next_query_row();
    sqc.finish_query();

    new_query_execute(&sqc, "select * from t");
    sqc.next_query_row();
    try
    {
        sqc.get_int("y");
        assert(false);
    }
    catch(std::string &se)
    {
        assert(se.find(ERR_SQMC) != string::npos);
    }

    return test_pass;
}

// call get function in the middle of insert statement
bool negative_testing_get_in_insert() {

    run_direct_query(&sqc, "create or replace table t (x int not null)");
    new_query_execute(&sqc, "insert into t values (?)");
    sqc.set_int(0, 1);
    try
    {
        sqc.get_int(0);
        assert(false);
    }
    catch(std::string &se)
    {
        assert(se.find(ERR_SQMC) != string::npos);
    }

    return test_pass;
}

// call set function in the middle of select statement
bool negative_testing_set_in_select() {

    run_direct_query(&sqc, "create or replace table t (x int not null)");
    new_query_execute(&sqc, "select * from t");
    try
    {
        sqc.set_int(0, 1);
        assert(false);
    }
    catch(std::string &se)
    {
        assert(se.find(ERR_SQMC) != string::npos);
    }

    return test_pass;
}

bool negative_testing_prepare_statement_mismatch_in_param() {

    run_direct_query(&sqc, "create or replace table t (x int not null)");
    try
    {
        new_query_execute(&sqc, "insert into t values (?,?)");
        assert(false);
    }
    catch(std::string &se)
    {
        assert(se.find(ERR_SQMC) != string::npos);
    }

    return test_pass;
}

bool negative_testing_int_insert_overwrite() {

    run_direct_query(&sqc, "create or replace table t(x int not null,y int)");
    new_query_execute(&sqc, "insert into t values (?,?)");
    sqc.set_int(0,0);
    sqc.set_int(1,0);
    sqc.next_query_row();
    sqc.set_int(0,0);
    sqc.set_int(1,0);
    try
    {
        sqc.set_int(0,3);
        sqc.set_int(1,5);
        sqc.next_query_row();
    }
    catch(std::string &se)
    {
        assert(se.find(ERR_SQMC) != string::npos);
    }

    return test_pass;
}

bool negative_testing_int_insert_null_overwrite() {

    run_direct_query(&sqc, "create or replace table t(x int)");
    new_query_execute(&sqc, "insert into t values (?)");
    sqc.set_int(0,0);
    try
    {
        sqc.set_null(0);
        sqc.next_query_row();
        sqc.finish_query();
    }
    catch(std::string &se)
    {
        assert(se.find(ERR_SQMC) != string::npos);
    }

    return test_pass;
}

bool negative_testing_too_many_next_row_calls_on_select() {

    run_direct_query(&sqc, "create or replace table t(x int)");
    new_query_execute(&sqc, "insert into t values (?)");
    sqc.set_int(0,666);
    sqc.next_query_row();
    sqc.finish_query();
    new_query_execute(&sqc, "select * from t");
    try
    {
        for(;;) sqc.next_query_row();
    }
    catch(std::string &se)
    {
        assert(se.find(ERR_SQMC) != string::npos);
    }

    return test_pass;
}


int main () {

    //  --- Setup for all tests
    setup_tests();
    try {
        simple();
    }
    catch (std::string e) {
        puts(e.c_str());
    }
}
