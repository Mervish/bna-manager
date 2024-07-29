#include "scenario.h"

#include <boost/range/adaptor/transformed.hpp>

namespace {
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
  if (!json_obj.contains("path")) {
    return {false, "missing 'path' field."};
  }
  bool const has_files = json_obj.contains("files");
  bool const has_filter = json_obj.contains("filter");
  if (!has_files && !has_filter) {
    return {false, "missing 'files' or 'filter' field."};
  }
  if (has_files && has_filter) {
    return {false, "'files' and 'filter' fields are mutually exclusive."};
  }
  try {
    path.assign(json_obj.at("path").as_string().c_str());
    jsonArrayToIntArray(files, json_obj.at("files").as_array(), jsonValueToString);
  } catch (...) {
    return {false, "invalid field type."};
  }
  return {true, ""};
}

boost::json::value OperationEntry::toJSON() const {
  auto json_obj = boost::json::object();
  json_obj["path"] = path.string();
  auto files_json = boost::json::array();
  for (auto const& filepath : files) {
    files_json.emplace_back(filepath.string());
  }
  return json_obj;
}

std::pair<bool, std::string> OperationScenario::fromJSON(const boost::json::value& json) {
  if(!json.is_object()) {
    return {false, "Invalid script-file structure."};
  }
  auto const& json_obj = json.as_object();

  //process loose files
  if(json_obj.contains("bna")) {
    auto const& bna_json = json_obj.at("bna").as_array();
    for (auto const& entry : bna_json) {
      auto& op_entry = entries.emplace_back();
      if(auto res = op_entry.fromJSON(entry); !res.first) {
        return res;
      }
    }
  }
  //process loose files
  if(json_obj.contains("loose")) {
    auto const& loose_json = json_obj.at("loose").as_array();
    for (auto const& entry : loose_json) {
      if (!entry.is_string()) {
        return {false, "Invalid field type for 'loose'."};
      }
      loose.emplace_back(entry.as_string().c_str());
    }
  }
  return {true, ""};
}

boost::json::value OperationScenario::toJSON() const
{
  auto json_obj = boost::json::object();
  json_obj["loose"] = boost::json::array(loose.begin(), loose.end());
  auto const entries_trans = entries | boost::adaptors::transformed([](OperationEntry const& entry){
                               return entry.toJSON();
                             });
  json_obj["entries"] = boost::json::array(entries_trans.begin(), entries_trans.end());
  return json_obj;
}

}
}
