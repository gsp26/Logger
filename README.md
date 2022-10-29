# Logger
Header only Async C++ Logger with log rotation

# Example usage
```
#include "Logger.h"

using namespace std;

std::mutex Output2FILE::mtx;
ThreadSafeQueue<std::string> Output2FILE::logQueue;
Level Output2FILE::logLevelNew = Level::INFO;
std::string Output2FILE::logFileName = "";
std::string Output2FILE::logPath = "";
std::string Output2FILE::logFileNamewithDate = "";

int main(int argc, char* argv[])
{
    //Logger
    Output2FILE::setLogFileName("/var/log/","Log.out"); // give any log path and file name
    FILE * pFile = std::fopen(Output2FILE::getLogFileName().c_str(),"a");
    if(pFile == NULL)
    {
        std::cout << "cannot open log file " << std::endl;
        return 0;
    }
    Output2FILE::SetStream(pFile);
    std::thread loggerThread(Output2FILE::flushLog);
    LOG_INFO("className","functionName","started logger thread");
    Output2FILE::setLogLevel("DEBUG");
    
    LOG_DEBUG("className","functionName","started logger thread");
    LOG_ERROR("className","functionName","started logger thread");
    TRACE_FUNCTION_ENTRY("className","functionName");
    TRACE_FUNCTION_ENTRY("className","functionName");

    return 0;
}
```
    
