#include "scenario.h"

#include <fstream>

#include <boost/range/adaptor/transformed.hpp>

namespace {
constexpr auto entries_literal = "entries";
constexpr auto loose_literal = "loose";
constexpr auto files_literal = "files";
constexpr auto path_literal = "path";

std::string jsonValueToString(boost::json::value const& json) { return json.as_string().c_str(); }

template <typename Val, typename Pred>
void jsonArrayToIntArray(std::vector<Val>& array, boost::json::array const& json, Pred callback) {
  array.clear();
  for (auto const& entry : json) {
    array.push_back(callback(entry));
  }
}
}  // namespace

namespace imas {
namespace file {

std::pair<bool, std::string> OperationEntry::fromJSON(const boost::json::value& json) {
  if (!json.is_object()) {
    return {false, "invalid entry structure."};
  }
  auto const& json_obj = json.as_object();
  // check if required fields are present
  if (!json_obj.contains(path_literal)) {
    return {false, "missing 'path' field."};
  }
  bool const has_files = json_obj.contains(files_literal);
  bool const has_filter = json_obj.contains("filter");
  if (!has_files && !has_filter) {
    return {false, "missing 'files' or 'filter' field."};
  }
  if (has_files && has_filter) {
    return {false, "'files' and 'filter' fields are mutually exclusive."};
  }
  try {
    path.assign(json_obj.at(path_literal).as_string().c_str());
    jsonArrayToIntArray(files, json_obj.at(files_literal).as_array(), jsonValueToString);
  } catch (...) {
    return {false, "invalid field type."};
  }
  return {true, ""};
}

boost::json::value OperationEntry::toJSON() const {
  auto json_obj = boost::json::object();
  json_obj[path_literal] = path.string();
  auto files_json = boost::json::array();
  for (auto const& filepath : files) {
    files_json.emplace_back(filepath.string());
  }
  json_obj[files_literal] = files_json;
  return json_obj;
}

std::pair<bool, std::string> OperationScenario::fromJSON(const boost::json::value& json) {
  if(!json.is_object()) {
    return {false, "Invalid script-file structure."};
  }
  auto const& json_obj = json.as_object();

  //process loose files
  if(json_obj.contains(entries_literal)) {
    auto const& bna_json = json_obj.at(entries_literal).as_array();
    for (auto const& entry : bna_json) {
      auto& op_entry = entries.emplace_back();
      if(auto res = op_entry.fromJSON(entry); !res.first) {
        return res;
      }
    }
  }
  //process loose files
  if(json_obj.contains(loose_literal)) {
    auto const& loose_json = json_obj.at(loose_literal).as_array();
    for (auto const& entry : loose_json) {
      if (!entry.is_string()) {
        return {false, "Invalid field type for 'loose'."};
      }
      loose.emplace_back(entry.as_string().c_str());
    }
  }
  return {true, ""};
}

std::pair<bool, std::string> OperationScenario::fromFile(std::filesystem::path const& filepath) {
  std::ifstream stream(filepath);
  if (!stream.is_open()) {
    return {false, "Failed to open file."};
  }
  boost::json::value json;
  stream >> json;
  return OperationScenario::fromJSON(json);
}

boost::json::value OperationScenario::toJSON() const
{
  auto json_obj = boost::json::object();
  json_obj[loose_literal] = boost::json::array(loose.begin(), loose.end());
  auto const entries_trans = entries | boost::adaptors::transformed([](OperationEntry const& entry){
                               return entry.toJSON();
                             });
  json_obj[entries_literal] = boost::json::array(entries_trans.begin(), entries_trans.end());
  return json_obj;
}

}
}
