#ifndef LOGGING_H_INCLUDED
#define LOGGING_H_INCLUDED

#include <sstream>
#include <mutex>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace programmerjake
{
namespace voxels
{
class Logger
{
    std::mutex theLock;
    std::shared_ptr<std::wostream> logStream;
    void postMessage(std::wstring msg)
    {
        std::unique_lock<std::mutex> lockIt(theLock);
        if(logStream == nullptr)
            std::wcout << std::move(msg) << std::flush;
        else
            *logStream << std::move(msg) << std::flush;
    }
public:
    constexpr Logger()
    {
    }
    Logger(const Logger &) = delete;
    const Logger &operator =(const Logger &) = delete;

    class LogMessageHelper
    {
    private:
        std::wostringstream stream;
        Logger &logger;
        LogMessageHelper(Logger &logger)
            : logger(logger)
        {
        }
    public:
        ~LogMessageHelper()
        {
            logger.postMessage(stream.str());
        }
        LogMessageHelper(const LogMessageHelper &) = delete;
        LogMessageHelper(LogMessageHelper &&rt)
            : stream(rt.stream.str()), logger(rt.logger)
        {
        }
        const LogMessageHelper &operator =(const LogMessageHelper &) = delete;
        template <typename T>
        friend std::wostream &operator <<(LogMessageHelper &&lmh, T v)
        {
            return lmh.stream << std::forward<T>(v);
        }
        template <typename T>
        friend LogMessageHelper operator <<(Logger &logger, T v);
    };

    template <typename T>
    friend LogMessageHelper operator <<(Logger &logger, T v);

    void setLogStream(std::shared_ptr<std::wostream> logStream)
    {
        std::unique_lock<std::mutex> lockIt(theLock);
        this->logStream = std::move(logStream);
    }
};

template <typename T>
inline Logger::LogMessageHelper operator <<(Logger &logger, T v)
{
    Logger::LogMessageHelper retval(logger);
    retval.stream << std::forward<T>(v);
    return std::move(retval);
}


extern Logger debugLog;
}
}

#endif // LOGGING_H_INCLUDED
