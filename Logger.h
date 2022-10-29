#include<mutex>
#include<sstream>
#include<fstream>
#include<chrono>
#include <ctime>
#include<thread>


#include "ThreadSafeQueue.h"


#ifndef LOGGER_H
#define LOGGER_H



enum class Level {FATAL,ERROR,WARN,INFO,DEBUG,TRACE};

template<typename OutputPolicy>
class Logger
{
public:
    Logger();
    virtual ~Logger();
    std::ostringstream & Get(Level,std::string,std::string,std::string,std::string);
public:
    static Level ReportingLevel();
protected:
    std::ostringstream os;
private:
    Logger(const Logger&);
    Logger& operator=(const Logger&);
private:
    Level logLevel;
};

template <typename OutputPolicy>
Logger<OutputPolicy>::Logger()
{
    Logger::logLevel = Level::DEBUG;
}

template <typename OutputPolicy>
Logger<OutputPolicy>::~Logger()
{
    OutputPolicy::push(os.str());
}

template <typename OutputPolicy>
std::ostringstream & Logger<OutputPolicy>::Get(Level level,std::string levelStr,std::string className,std::string functionName,std::string logMsg)
{
    char currentTime[23];
    uint64_t micros = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::
                  now().time_since_epoch()).count();
    unsigned int microSec = micros%1000000;
    time_t now = time(0);
    tm *ltm = localtime(&now);
    strftime(currentTime,sizeof(currentTime), "%Y%m%dT%H%M%S", ltm);
    sprintf(currentTime, "%s.%u", currentTime, microSec);

    os << currentTime << "|" << std::this_thread::get_id()
       << "|" << levelStr <<"|";
    os << className <<"|"<<functionName<<"|"<<logMsg<<std::endl;
    this->logLevel = level;

    return os;
}


template <typename OutputPolicy>
Level Logger<OutputPolicy>::ReportingLevel(){ return OutputPolicy::getLogLevel() ;}



class Output2FILE // implementation of OutputPolicy
{
public:
    static void Output(const std::string& msg);
    static void SetStream(FILE* pFile);
    static void push(const std::string&msg);
    static void flushLog();
    static Level getLogLevel();
    static void setLogLevel(std::string &value);
    static void setLogFileName(std::string path,std::string name);
    static std::string getLogFileName();
    static std::string getTodayLogFileName();
    static std::string getDateString();

private:
    static FILE*& StreamImpl();
    static std::mutex mtx;
    static Level logLevelNew;
    static std::string logFileName;
    static std::string logPath;
    static std::string logFileNamewithDate;
    static ThreadSafeQueue<std::string> logQueue;
};



inline FILE*& Output2FILE::StreamImpl()
{
    static FILE* pStream = stderr;
    return pStream;
}

inline Level Output2FILE::getLogLevel()
{
    return logLevelNew;
}

inline void Output2FILE::setLogLevel(std::string &value)
{
    if(value == "TRACE")
        logLevelNew = Level::TRACE;
    else if(value == "DEBUG")
        logLevelNew = Level::DEBUG;
    else if(value == "INFO")
        logLevelNew = Level::INFO;
    else if(value == "WARN")
        logLevelNew = Level::WARN;
    else if(value == "ERROR")
        logLevelNew = Level::ERROR;
    else if(value == "FATAL")
        logLevelNew = Level::FATAL;
    else
        logLevelNew = Level::INFO;
}

inline void Output2FILE::setLogFileName(std::string path,std::string name)
{
    logFileName = name;
    logPath = path;
    logFileNamewithDate = logPath + "/" + getDateString() + "_"+ logFileName;
}

inline std::string Output2FILE::getLogFileName()
{
    return logFileNamewithDate;
}

inline std::string Output2FILE::getTodayLogFileName()
{
    return  logPath + "/" + getDateString() + "_"+ logFileName;
}

inline std::string Output2FILE::getDateString()
{
    time_t l_CurrentTime = time(0);
    char l_tempBuf[80] = {"\0"};
    struct tm l_tstructLocalTime;
    l_tstructLocalTime = *localtime(&l_CurrentTime);

    strftime(l_tempBuf,sizeof(l_tempBuf),"%d-%b-%Y" ,&l_tstructLocalTime);
    std::string l_strTimeFormt(l_tempBuf);
    return l_strTimeFormt;
}

inline void Output2FILE::SetStream(FILE* pFile)
{
    std::lock_guard<std::mutex> lock(mtx);
    StreamImpl() = pFile;
}
inline void Output2FILE::Output(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(mtx);
    FILE* pStream = StreamImpl();
    if (!pStream)
        return;
    fprintf(pStream, "%s", msg.c_str());
    fflush(pStream);
}
inline void Output2FILE::push(const std::string& msg)
{
    logQueue.push(msg);
}
inline void Output2FILE::flushLog()
{
    std::string msg;
    while(true){
        logQueue.wait_and_pop(msg);
        if(!msg.empty()){
            if(getLogFileName() != getTodayLogFileName()){
                                std::string logDelCommand = "find " + logPath + "*" + logFileName + "* -mtime +5  -delete";
                                system(logDelCommand.c_str());
                setLogFileName(logPath,logFileName);
                FILE * pFile = std::fopen(logFileNamewithDate.c_str(),"a");
                if(pFile != NULL)
                   SetStream(pFile);
            }
            FILE* pStream = StreamImpl();
            //log rotation after 1GB (1073741824 bytes)
            if(ftell(pStream) > 1073741824){
                char timestamp[50]{ 0 };
                std::time_t time = std::time(nullptr);
                std::strftime(timestamp, 30, "%H%M%S", std::localtime(&time));
                std::string newName = logFileNamewithDate + "_" +timestamp;
                std::rename(logFileNamewithDate.c_str(),newName.c_str());
                setLogFileName(logPath,logFileName);
                FILE * pFile = std::fopen(logFileNamewithDate.c_str(),"a");
                if(pFile != NULL)
                    SetStream(pFile);
            }
            if (!pStream)
                return;
            fprintf(pStream, "%s", msg.c_str());
            fflush(pStream);
        }
    }
}


typedef Logger<Output2FILE> FILELog;

#define LOG_FATAL(C,F,M) \
    if (Level::FATAL > FILELog::ReportingLevel()) ; \
    else FILELog().Get(Level::FATAL,"FATAL",C,F,M)

#define LOG_ERROR(C,F,M) \
    if (Level::ERROR > FILELog::ReportingLevel()) ; \
    else FILELog().Get(Level::ERROR,"ERROR",C,F,M)

#define LOG_WARN(C,F,M) \
    if (Level::WARN > FILELog::ReportingLevel()) ; \
    else FILELog().Get(Level::WARN,"WARN",C,F,M)

#define LOG_INFO(C,F,M) \
    if (Level::INFO > FILELog::ReportingLevel()) ; \
    else FILELog().Get(Level::INFO,"INFO",C,F,M)

#define LOG_DEBUG(C,F,M) \
    if (Level::DEBUG > FILELog::ReportingLevel()) ; \
    else FILELog().Get(Level::DEBUG,"DEBUG",C,F,M)


#define TRACE_FUNCTION_ENTRY(C,F) \
    if (Level::TRACE > FILELog::ReportingLevel()) ; \
    else FILELog().Get(Level::TRACE,"TRACE",C,F,"Function Entry")

#define TRACE_FUNCTION_EXIT(C,F) \
    if (Level::TRACE > FILELog::ReportingLevel()) ; \
    else FILELog().Get(Level::TRACE,"TRACE",C,F,"Function Exit")
#endif


// LOGGER_H
