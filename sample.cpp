#include <cstdio>
#include <cstdlib>
#include <string>
#include "connector.h"

using namespace std;

int main(int argc,char *argv[])
{
    try
    {
        std::vector<char> data={1,0,0,0};
        std::vector<uint64_t> column_sizes;
        std::vector<sqream::column> col_input,col_output;
        std::string query((argc>1)?argv[1]:"select 1");
        sqream::driver sqc;
        if(sqc.connect("localhost", 5000, false, "sqream", "sqream", "master"))
            printf("Connected non ssl\n");
        run_direct_query(&sqc, "select 1");
        
        if(sqc.connect("localhost", 5100, true, "sqream", "sqream", "master"))
            printf("Connected ssl\n");
        run_direct_query(&sqc, "select 1");
        
    }
    catch(std::string &err)
    {
        printf("%s\n",err.data());
        return EXIT_FAILURE;
    }
    
    printf("Done testing\n");
    return EXIT_SUCCESS;
}
