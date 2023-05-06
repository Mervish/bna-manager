
#include "pathsaver.h"

namespace {
constexpr auto savefile = "savedpath";
}

PathSaver::PathSaver(QObject *parent)
    : QObject{parent}
{
    QFile file(savefile);
    file.open(QIODevice::ReadOnly);
    QDataStream out(&file);
    out >> m_path_data;
}

PathSaver::~PathSaver() {
    QFile file(savefile);
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);
    out << m_path_data;
}

bool PathSaver::interrogate(QFileDialog& dialog) {
    auto const filter = dialog.nameFilters().first();
    dialog.setDirectory(m_path_data.value(filter, ""));
    auto const res = dialog.exec();
    if(!res) { return false; }
    auto const dir = dialog.directory();
    m_path_data[filter] = dir.absolutePath();
    return true;
}

