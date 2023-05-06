
#pragma once

#include <QFileDialog>
#include <QObject>

class PathSaver : public QObject
{
    Q_OBJECT
public:
    explicit PathSaver(QObject *parent = nullptr);
    ~PathSaver();
    bool interrogate(QFileDialog& dialog);

signals:

private:
    QMap<QString, QString> m_path_data;
};
