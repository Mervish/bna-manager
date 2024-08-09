#pragma once

#include <algorithm>
#include <string>
#include <ranges>
#include <filesystem>
#include <vector>

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

bool isEmptyRecursive(std::filesystem::path dir) {
  return std::ranges::none_of(std::filesystem::recursive_directory_iterator(dir),
                           [](std::filesystem::path const &path) {
                             return std::filesystem::is_regular_file(path);
                           });
}

template<typename _Pred>
void iterateFiles(std::filesystem::path const& gamepath, std::string const& type,
                _Pred const& callback) {
  auto const iter = std::filesystem::recursive_directory_iterator(gamepath);
  for (auto const& file :
       iter
           | std::views::filter(
                  [&type](std::filesystem::directory_entry const& dir) {
                   if (dir.is_regular_file()) {
                     return dir.path().extension() == type;
                   }
                   return false;
                 })) {
    callback(file.path());
  }
}

std::vector<std::string> collectFilepaths(std::filesystem::path const& gamepath, std::string const& type) {
  std::vector<std::string> files;
  iterateFiles(gamepath, type, [&files](std::filesystem::path const& path) {
    files.push_back(path.string());
  });
  return files;
}

}
}
