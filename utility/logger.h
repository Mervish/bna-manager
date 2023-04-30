
#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QPlainTextEdit>

class Logger;

enum class LogMessageType {
    Info,
    Warning,
    Error,
    Exception
};

class logInstance : QObject {
    friend Logger;
    void log(LogMessageType type, const QString &message);
private:
    explicit logInstance(QObject *parent, QString const& name);

    //parameters
    QString m_name; //Instance name
};

class Logger : public QObject
{
    Q_OBJECT
public:

    logInstance makeInstance(QString const& name);
    void setTextEdit(QPlainTextEdit *log);
    void log(LogMessageType type, const QString &message);
    void log(logInstance &instance, LogMessageType type, const QString &message);

    static Logger* getLogger();
signals:

private:
    explicit Logger(QObject *parent = nullptr);
    QPlainTextEdit* m_log;
};

#endif // LOGGER_H
