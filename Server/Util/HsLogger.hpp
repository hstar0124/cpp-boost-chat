#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <sys/stat.h> // 디렉토리 생성용
#include <direct.h>   // _mkdir 함수용

// 로그 레벨 정의
enum class LogLevel
{
    DEBUG,
    INFO,
    WARNING,
    ERR
};

// 파일 로그 기록 주기 정의
enum class LogFilePeriod
{
    DAILY,
    MONTHLY,
    YEARLY
};


// 로그 기록 인터페이스
class ILogger
{
public:
    virtual ~ILogger() = default;

    // 로그 기록 함수
    virtual void Log(LogLevel level, const std::string& message, const std::string& filename, int line) = 0;

    // 전역 로그 레벨 설정
    static void SetLogLevel(LogLevel level)
    {
        m_GlobalLogLevel = level;
    }

    // 전역 로그 레벨 가져오기
    static LogLevel GetLogLevel()
    {
        return m_GlobalLogLevel;
    }

protected:
    // 전역 로그 레벨 변수
    static LogLevel m_GlobalLogLevel;

    // 현재 시간 문자열 반환
    static std::string GetCurrentTime()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now;
        localtime_s(&tm_now, &time_t_now);

        std::ostringstream oss;
        oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    // 로그 레벨 enum 값을 문자열로 변환
    static std::string LogLevelToString(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERR: return "ERROR";
        default: return "";
        }
    }
};

// 전역 로그 레벨 변수 초기화
LogLevel ILogger::m_GlobalLogLevel = LogLevel::DEBUG;

// 콘솔 로그 클래스
class ConsoleLogger : public ILogger
{
public:
    void Log(LogLevel level, const std::string& message, const std::string& filename, int line) override;

private:
    std::mutex  m_ConsoleMutex;
    std::string FormatLogMessage(LogLevel level, const std::string& message, const std::string& filename, int line);
};

// 파일 로그 클래스
class FileLogger : public ILogger
{
public:
    FileLogger(const std::string& logFilename, LogFilePeriod period);

    void Log(LogLevel level, const std::string& message, const std::string& filename, int line) override;

private:
    std::string FormatLogMessage(LogLevel level, const std::string& message, const std::string& filename, int line);
    std::string AppendPeriodToFilename(const std::string& baseFilename);

private:
    std::ofstream   m_File;
    std::mutex      m_FileMutex;
    std::string     m_LogFilename;
    LogFilePeriod   m_Period;

};

// 멀티 로거 클래스 (여러 로거를 관리)
class HsLogger : public ILogger
{
public:
    HsLogger() = default;
    ~HsLogger() = default;

    void AddLogger(std::shared_ptr<ILogger> logger)
    {
        m_Loggers.push_back(logger);
    }

    void Log(LogLevel level, const std::string& message, const std::string& filename, int line) override
    {
        for (const auto& logger : m_Loggers)
        {
            logger->Log(level, message, filename, line);
        }
    }

private:
    std::vector<std::shared_ptr<ILogger>> m_Loggers;
};

// 전역 로거 변수
extern std::shared_ptr<HsLogger> g_Logger;

// 전역 함수로 로거 초기화
std::shared_ptr<HsLogger> InitializeLoggers(LogLevel globalLogLevel, bool useConsoleLogger, bool useFileLogger, const std::string& logFilename, LogFilePeriod period);

// 매크로 정의
#define LOG(level, message) \
    if (g_Logger) g_Logger->Log(level, message, __FILE__, __LINE__)

// ---- 함수 정의 ----

// 콘솔 로그 기록 함수
inline void ConsoleLogger::Log(LogLevel level, const std::string& message, const std::string& filename, int line)
{
    if (level >= ILogger::GetLogLevel())
    {
        std::lock_guard<std::mutex> lock(m_ConsoleMutex);
        std::cout << FormatLogMessage(level, message, filename, line) << std::endl;
    }
}

// 콘솔 로그 메시지 포맷팅
inline std::string ConsoleLogger::FormatLogMessage(LogLevel level, const std::string& message, const std::string& filename, int line)
{
    std::ostringstream oss;
    oss << "[" << LogLevelToString(level) << "] [" << GetCurrentTime() << "] [" << message << "] [" << filename << ":" << line << "]";
    return oss.str();
}


// 파일 로거 생성자
inline FileLogger::FileLogger(const std::string& logFilename, LogFilePeriod period)
    : m_LogFilename(logFilename), m_Period(period)
{
    // logs 디렉토리 생성
#if defined(_WIN32)
    _mkdir("logs");
#else 
    mkdir("logs", 0777);
#endif
    // logs 디렉토리 내에 파일 이름 설정
    std::string filenameWithPeriod = "logs/" + AppendPeriodToFilename(logFilename);

    m_File.open(filenameWithPeriod, std::ios::app);
    if (!m_File.is_open())
    {
        throw std::runtime_error("Could not open log file: " + filenameWithPeriod);
    }
}

// 파일 로그 기록 함수
inline void FileLogger::Log(LogLevel level, const std::string& message, const std::string& filename, int line)
{
    if (level >= ILogger::GetLogLevel())
    {
        std::lock_guard<std::mutex> lock(m_FileMutex);
        m_File << FormatLogMessage(level, message, filename, line) << std::endl;
    }
}

// 파일 로그 메시지 포맷팅
inline std::string FileLogger::FormatLogMessage(LogLevel level, const std::string& message, const std::string& filename, int line)
{
    std::ostringstream oss;
    oss << "[" << LogLevelToString(level) << "] [" << GetCurrentTime() << "] [" << message << "] [" << filename << ":" << line << "]";
    return oss.str();
}

// 파일 이름에 로그 기록 주기 추가
inline std::string FileLogger::AppendPeriodToFilename(const std::string& baseFilename)
{
    std::string filenameWithPeriod = baseFilename;
    switch (m_Period) {
    case LogFilePeriod::DAILY:
        filenameWithPeriod += "_" + ILogger::GetCurrentTime().substr(0, 10);
        break;
    case LogFilePeriod::MONTHLY:
        filenameWithPeriod += "_" + ILogger::GetCurrentTime().substr(0, 7);
        break;
    case LogFilePeriod::YEARLY:
        filenameWithPeriod += "_" + ILogger::GetCurrentTime().substr(0, 4);
        break;
    }

    return filenameWithPeriod + ".txt";
}

// 전역 로거 변수
std::shared_ptr<HsLogger> g_Logger = nullptr;

// 전역 함수로 로거 초기화
inline std::shared_ptr<HsLogger> InitializeLoggers(LogLevel globalLogLevel, bool useConsoleLogger, bool useFileLogger, const std::string& logFilename, LogFilePeriod period)
{
    auto multiLogger = std::make_shared<HsLogger>();

    ILogger::SetLogLevel(globalLogLevel);

    if (useConsoleLogger) {
        multiLogger->AddLogger(std::make_shared<ConsoleLogger>());
    }

    if (useFileLogger) {
        multiLogger->AddLogger(std::make_shared<FileLogger>(logFilename, period));
    }

    g_Logger = multiLogger;

    return multiLogger;
}
