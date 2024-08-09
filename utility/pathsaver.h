
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
    QString m_dir_path;
    QMap<QString, QString> m_path_data;
};
