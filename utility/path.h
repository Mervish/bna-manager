#pragma once

#include <string>
#include <filesystem>
#include <QString>

namespace imas {
namespace path {


std::filesystem::path changeExtension(std::filesystem::path const& origin, std::string const& ext) {
  auto path = origin;
  path.replace_extension(ext);
  return path;
}

std::filesystem::path applySuffix(std::filesystem::path const& origin, std::string const& suffix) {
  auto path = std::filesystem::absolute(origin.parent_path()) / origin.stem();
  path += suffix;
  path += origin.extension();
  return path;
}

/*QString changeExtension(QString const& path) {

}

QString applySuffix(QString const& path) {

}*/

QString removeExtension(QString const& path) {
  return path.mid(path.lastIndexOf('/') + 1, path.lastIndexOf('.') - path.lastIndexOf('/') - 1);
}

bool isEmptyRecursive(std::filesystem::path dir) {
  return std::ranges::none_of(std::filesystem::recursive_directory_iterator(dir),
                           [](std::filesystem::path const &path) {
                             return std::filesystem::is_regular_file(path);
                           });
}


}
}
