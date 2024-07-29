#pragma once

#include <QObject>
#include <QPlainTextEdit>

class Logger : public QObject
{
    Q_OBJECT
public:
    enum class LogMessageType { Info, Warning, Error, Critical };
    void setTextEdit(QPlainTextEdit *log);
    void log(QString const &message);
    void info(const QString &message);
    void warning(const QString &message);
    void error(const QString &message);
    void critical(const QString &message);

    static Logger *getLogger();
signals:

private:
    explicit Logger(QObject *parent = nullptr);
    static QString toString(LogMessageType type);
    void log(LogMessageType type, const QString &message);
    QPlainTextEdit *m_log;
};
