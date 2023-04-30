
#include "logger.h"

namespace {
QString toString(LogMessageType type) {
    switch (type) {
    case LogMessageType::Info:
        return "Info:";
    case LogMessageType::Warning:
        return "Warning:";
    case LogMessageType::Error:
        return "Error:";
    case LogMessageType::Exception:
        return "Exception:";
    default:
        Logger::getLogger()->log(LogMessageType::Exception, "Logger::toString: Unknown type");
    }
}
}


logInstance Logger::makeInstance(const QString& name) {
    return logInstance{this, name};
}

void Logger::setTextEdit(QPlainTextEdit* log){
    m_log = log;
}

void Logger::log(LogMessageType type, const QString& message){
    m_log->appendPlainText(toString(type) + message);
}

void Logger::log(logInstance& instance, LogMessageType type, const QString& message){
    m_log->appendPlainText(toString(type) + instance.m_name + ':' + message);
}

Logger* Logger::getLogger() {
    static Logger logger;
    return &logger;
}

Logger::Logger(QObject* parent)
    : QObject{parent}
{

}

void logInstance::log(LogMessageType type, const QString& message){
    Logger::getLogger()->log(type, message);
}

logInstance::logInstance(QObject* parent, const QString& name) : QObject{parent}, m_name{name} {

}
