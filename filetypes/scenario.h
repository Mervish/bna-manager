#pragma once

#include "utility/result.h"

#include <filesystem>

#include <boost/json.hpp>

namespace imas {
namespace file {

enum class Filetypes { bna, bxr, nut, scb };

struct OperationEntry {
  std::filesystem::path path;
  std::vector<std::filesystem::path> files;
  Result fromJSON(boost::json::value const& json);
  boost::json::value toJSON() const;
};

struct OperationScenario {
  std::vector<OperationEntry> entries;
  std::vector<std::string> loose;
  Result fromJSON(boost::json::value const& json);
  Result fromFile(std::filesystem::path const& filepath);
  boost::json::value toJSON() const;
};

}  // namespace file
}  // namespace imas
