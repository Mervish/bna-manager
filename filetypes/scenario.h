#pragma once

#include <filesystem>
#include <set>

#include <boost/json.hpp>

namespace imas {
namespace file {

enum class Filetypes { bna, bxr, nut, scb };

struct OperationEntry {
  std::filesystem::path path;
  std::vector<std::filesystem::path> files;
  std::pair<bool, std::string> fromJSON(boost::json::value const& json);
  boost::json::value toJSON() const;
};

struct OperationScenario {
  std::vector<OperationEntry> entries;
  std::vector<std::string> loose;
  std::pair<bool, std::string> fromJSON(boost::json::value const& json);
  std::pair<bool, std::string> fromFile(std::filesystem::path const& filepath);
  boost::json::value toJSON() const;
};

}  // namespace file
}  // namespace imas
