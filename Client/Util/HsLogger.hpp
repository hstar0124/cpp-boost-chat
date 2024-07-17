#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <iomanip>
#include <filesystem>
#include <mutex>
#include <memory>

// Enum for log levels
enum class LogLevel
{
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
};

// Enum for log periods
enum class LogPeriod
{
    YEAR,
    MONTH,
    DAY
};

// Base class for log output
class LogOutput
{
public:
    virtual ~LogOutput() = default;
    virtual void write(const std::string& message) = 0;
};

// Derived class for console output
class ConsoleOutput : public LogOutput
{
public:
    void write(const std::string& message) override
    {
        std::cout << message << std::endl;
    }
};

// Derived class for file output
class FileOutput : public LogOutput
{
public:
    explicit FileOutput(const std::string& filename)
        : logFileName(filename)
    {
        logFile.open(logFileName, std::ios_base::app);
        if (!logFile.is_open())
        {
            std::cerr << "Failed to open log file: " << logFileName << std::endl;
        }
    }

    void write(const std::string& message) override
    {
        logFile << message << std::endl;
        logFile.flush(); // Ensure the message is written immediately
    }

private:
    std::ofstream logFile;
    std::string logFileName;
};

// Logger class
class Logger
{
public:
    Logger(LogLevel level = LogLevel::LOG_INFO)
        : logLevel(level) {}

    void init(LogLevel level, LogPeriod period, bool logToFile, bool logToConsole)
    {
        std::lock_guard<std::mutex> lock(loggerMutex);
        logLevel = level;

        if (logToFile)
        {
            std::filesystem::create_directory("logs");
            updateLogFile(period);
            fileOutput = std::make_unique<FileOutput>(currentLogFileName);
        }

        if (logToConsole)
        {
            consoleOutput = std::make_unique<ConsoleOutput>();
        }
    }

    template<typename... Args>
    void log(LogLevel level, const std::string& message, const char* file, int line, Args... args)
    {
        std::lock_guard<std::mutex> lock(loggerMutex);
        if (level >= logLevel)
        {
            std::stringstream logStream;
            logStream << "[" << logLevelToString(level) << " " << currentDateTime() << "] "
                << format(message, args...) << " (" << file << ":" << line << ")";

            if (fileOutput)
            {
                fileOutput->write(logStream.str());
            }

            if (consoleOutput)
            {
                consoleOutput->write(logStream.str());
            }
        }
    }

    static Logger& instance()
    {
        static Logger logger;
        return logger;
    }

private:
    LogLevel logLevel;
    std::unique_ptr<LogOutput> fileOutput;
    std::unique_ptr<LogOutput> consoleOutput;
    std::mutex loggerMutex; // Mutex for logger operations
    std::string currentLogFileName;

    std::string logLevelToString(LogLevel level) const
    {
        switch (level)
        {
        case LogLevel::LOG_DEBUG: return "DEBUG";
        case LogLevel::LOG_INFO: return "INFO";
        case LogLevel::LOG_WARN: return "WARN";
        case LogLevel::LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
        }
    }

    std::string currentDateTime() const
    {
        std::time_t now = std::time(nullptr);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &now);
#else
        localtime_r(&now, &tm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    void updateLogFile(LogPeriod period)
    {
        std::time_t now = std::time(nullptr);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &now);
#else
        localtime_r(&now, &tm);
#endif
        std::ostringstream filename;
        filename << "logs/log_";
        switch (period)
        {
        case LogPeriod::YEAR:
            filename << std::put_time(&tm, "%Y");
            break;
        case LogPeriod::MONTH:
            filename << std::put_time(&tm, "%Y%m");
            break;
        case LogPeriod::DAY:
            filename << std::put_time(&tm, "%Y%m%d");
            break;
        }
        filename << ".log";

        currentLogFileName = filename.str();
    }

    template<typename... Args>
    std::string format(const std::string& format, Args... args)
    {
        size_t size = snprintf(nullptr, 0, format.c_str(), args...) + 1;
        std::unique_ptr<char[]> buf(new char[size]);
        snprintf(buf.get(), size, format.c_str(), args...);
        return std::string(buf.get(), buf.get() + size - 1);
    }
};

// Macro definitions for logging
#define LOG_DEBUG(message, ...) \
    Logger::instance().log(LogLevel::LOG_DEBUG, message, __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_INFO(message, ...) \
    Logger::instance().log(LogLevel::LOG_INFO, message, __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_WARN(message, ...) \
    Logger::instance().log(LogLevel::LOG_WARN, message, __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_ERROR(message, ...) \
    Logger::instance().log(LogLevel::LOG_ERROR, message, __FILE__, __LINE__, ##__VA_ARGS__)

