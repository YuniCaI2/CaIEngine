//
// Created by 51092 on 25-8-26.
//

#ifndef LOGGER_H
#define LOGGER_H

#include<string>

#include "PublicEnum.h"
#include<format>
#include<source_location>
#include<chrono>
#include <mutex>
#include <queue>
#include <fstream>
#include <print>
#include <filesystem>

//此Logger类使用单例实现
class Logger {
public:
    void Run() {
        if (running_.load()) {
            return;
        }
        running_.store(true);
        thread_ = std::thread(&Logger::ProcessLoop, this);
    };

    void Stop() {
        if (running_.load()) {
            running_.store(false);
            condition_.notify_one();
            if (thread_.joinable()) {
                thread_.join();
            }
        }
    };

    static Logger &GetInstance() {
        static Logger instance;
        return instance;
    };

    struct LogMessage {
        LogLevel level;
        std::string message;
        std::source_location location; //记录行号，来自cpp20
        std::chrono::system_clock::time_point timeStamp; //时间戳
    };

    void SetPrintToConsole(bool s) {
        printToConsole = s;
    }

    void SetPrintToFile(bool s) {
        printToFile = s;
    }

    void SetFilePath(const std::string& path) {
        logPath = path;
    }

    template<typename... Args>
    void Error(std::format_string<Args...> fmt, Args&&... args) {
        log(std::source_location::current(), LogLevel::Error, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Warn(std::format_string<Args...> fmt, Args&&... args) {
        log(std::source_location::current(), LogLevel::Warn,fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void Trace(std::format_string<Args...> fmt, Args&&... args) {
        log(std::source_location::current(), LogLevel::Trace,fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void DeBug(std::format_string<Args...> fmt, Args&&... args) {
        log(std::source_location::current(), LogLevel::Debug,fmt, std::forward<Args>(args)...);
    }


private:
    enum class Color {
        RED,
        GREEN,
        BLUE,
        YELLOW,
    };

    static std::string LogLevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Debug:
                return "Debug";
            case LogLevel::Trace:
                return "Trace";
            case LogLevel::Warn:
                return "Warn";
            case LogLevel::Error:
                return "Error";
            default:
                return "Unknown";
        }
    }

    static std::string PrintSourceLocation(const std::source_location &location) {
        return std::format("file: {} line: {}", location.file_name(), location.line());
    }

    static std::string PrintTimeStamp(const std::chrono::system_clock::time_point &timeStamp) {
        return std::format("{:%Y-%m-%d %H:%M:%S}", timeStamp);
    }

    static std::string SetColor(Color color, std::string message) {
        if (color == Color::RED) {
            return RED + message + RESET;
        }
        if (color == Color::GREEN) {
            return GREEN + message + RESET;
        }
        if (color == Color::BLUE) {
            return BLUE + message + RESET;
        }
        if (color == Color::YELLOW) {
            return YELLOW + message + RESET;
        }
        return message;
    }

    static std::string SetColor(LogLevel level, std::string message) {
        if (level == LogLevel::Error) {
            return RED + message + RESET;
        }
        if (level == LogLevel::Debug) {
            return GREEN + message + RESET;
        }
        if (level == LogLevel::Trace) {
            return BLUE + message + RESET;
        }
        if (level == LogLevel::Warn) {
            return YELLOW + message + RESET;
        }
        return message;
    }

    template<typename... Args>
    void log(std::source_location location, LogLevel logLevel, std::format_string<Args...> fmt, Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto meg = std::format(fmt, std::forward<Args>(args)...);
        logQueue_.push(LogMessage(logLevel, meg, location, std::chrono::system_clock::now()));
        condition_.notify_one();
    }


    Logger() = default;

    void ProcessLoop() {
        while (running_.load() || !logQueue_.empty()) {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [&] { return !logQueue_.empty() || !running_.load(); });

            while (!logQueue_.empty()) {
                LogMessage log = logQueue_.front();
                logQueue_.pop();
                lock.unlock();

                //写入
                WriteLog(log);

                lock.lock();
            }
        }
    }

    void WriteLog(const LogMessage &logMessage) {
        if (printToFile) {
            std::ofstream of(logPath);
            std::println(of, "[{}] [{}] : \"{}\" \n from [{}] \n",
                         (LogLevelToString(logMessage.level)),
                         PrintTimeStamp(logMessage.timeStamp),
                         (logMessage.message), PrintSourceLocation(logMessage.location));
        }
        if (printToConsole) {
            std::println("[{}] [{}] : \"{}\" \n from [{}] \n",
                         SetColor(logMessage.level, LogLevelToString(logMessage.level)),
                         PrintTimeStamp(logMessage.timeStamp),
                         SetColor(logMessage.level, logMessage.message), PrintSourceLocation(logMessage.location));
        }
    }


    //Param
    std::string logPath{"../resources/log/log.txt"};
    std::mutex mutex_;
    std::atomic<bool> running_{false};
    std::queue<LogMessage> logQueue_;
    std::condition_variable condition_;
    std::thread thread_;
    bool printToConsole = true;
    bool printToFile = true;


    static constexpr const char *RESET = "\033[0m";
    static constexpr const char *RED = "\033[31m";
    static constexpr const char *GREEN = "\033[32m";
    static constexpr const char *YELLOW = "\033[33m";
    static constexpr const char *BLUE = "\033[34m";

    // 背景色
    static constexpr const char *BG_RED = "\033[41m";
    static constexpr const char *BG_GREEN = "\033[42m";
    static constexpr const char *BG_YELLOW = "\033[43m";

    // 样式
    static constexpr const char *BOLD = "\033[1m";
    static constexpr const char *DIM = "\033[2m";
    static constexpr const char *UNDERLINE = "\033[4m";
};

#define LOG Logger::GetInstance()

#endif //LOGGER_H
