
#include "logger.h"

namespace {

} // namespace

QString Logger::toString(Logger::LogMessageType type)
{
    switch (type) {
    case LogMessageType::Info:
        return "Info: ";
    case LogMessageType::Warning:
        return "Warning: ";
    case LogMessageType::Error:
        return "Error: ";
    case LogMessageType::Critical:
        return "Critical: ";
    default:
        Logger::getLogger()->log(LogMessageType::Critical, "Logger::toString: Unknown type");
    }
}

void Logger::setTextEdit(QPlainTextEdit* log){
    m_log = log;
}

void Logger::log(QString const& message){
    if(m_log) {
        m_log->appendPlainText(message);
    }
}

void Logger::log(LogMessageType type, const QString& message){
    log(toString(type) + message);
}

void Logger::info(const QString &message)
{
    log(LogMessageType::Info, message);
}

void Logger::warning(const QString &message)
{
    log(LogMessageType::Warning, message);
}

void Logger::error(const QString &message)
{
    log(LogMessageType::Error, message);
}

void Logger::critical(const QString &message)
{
    log(LogMessageType::Critical, message);
}

Logger* Logger::getLogger() {
    static Logger logger;
    return &logger;
}

Logger::Logger(QObject* parent)
    : QObject{parent}
{

}
