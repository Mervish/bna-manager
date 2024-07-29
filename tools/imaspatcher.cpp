
#include <iostream>
#include <set>

#include "filetypes/bna.h"
#include "filetypes/bxr.h"
#include "filetypes/nut.h"
#include "filetypes/scb.h"
#include "filetypes/scenario.h"
#include "utility/commandline.h"
#include "utility/filetype.h"

#include <fstream>

#include <boost/json.hpp>

// check if the patch folder exists
#define CHECK_PATCH_FOLDER(patch_folder) \
if (!std::filesystem::exists(patch_folder)) { \
  return {false, "Patch folder does not exist."}; \
}

constexpr auto help = "Idolm@ster patching tool."
"Usage: imaspatcher <command> <game folder> <script file> <patch folder>"
"Commands:"
"  extract - exports game files to the patch folder"
"  patch - patches game files with the files from the patch folder"
"  generate - generates a script file from the patch folder"
"  validate - validates the script file";

std::pair<bool, std::string> validate(imas::file::OperationScenario& scenario, std::filesystem::path const& game_folder, std::filesystem::path const& script_file) {
  std::ifstream stream(script_file, std::ios_base::binary);
  if (!stream.is_open()) {
    return {false, "Failed to read scenario file."};
  }
  boost::json::value scenario_json;
  stream >> scenario_json;
  return scenario.fromJSON(scenario_json);
}

std::pair<bool, std::string> validate(std::filesystem::path const& game_folder, std::filesystem::path const& script_file) {
  imas::file::OperationScenario scenario;
  return validate(scenario, game_folder, script_file);
}

std::pair<bool, std::string> extract(std::filesystem::path const& game_folder, std::filesystem::path const& script_file,
                                     std::filesystem::path const& patch_folder) {
  // create the patch folder if it doesn't exist
  if (!std::filesystem::exists(patch_folder)) {
    std::filesystem::create_directories(patch_folder);
  }
  imas::file::OperationScenario scenario;
  if(auto res = validate(scenario, game_folder, script_file); !res.first){
    return res;
  }
  for(auto const& entry: scenario.entries) {
    for(auto const& subentry: entry.files) {
      auto const ext_type = imas::filetype::getFileType(subentry.extension());
      auto const final_path = patch_folder / subentry;
      imas::file::BNA bna;
      if(auto res = bna.loadFromFile(entry.path); !res.first)
      {
         return res;
      }
      switch (ext_type) {
        case imas::filetype::type::bxr:
        {
          imas::file::BXR bxr;
          bxr.loadFromFile(subentry);
          bxr.extract(final_path);
        }
        break;
        case imas::filetype::type::nut:
        {
          imas::file::NUT nut;
          nut.LoadNUT(subentry);
          nut.ExportDDS(final_path);
        }
        break;
        case imas::filetype::type::scb:
        {
          imas::file::SCB scb;
          scb.loadFromFile(subentry);
          scb.extract(final_path);
        }
        break;
        default:
        return {false, "Unsupported file type."};
      }
    }
  }
  return {true, {}};
}

std::pair<bool, std::string> patch(std::filesystem::path const& game_folder, std::filesystem::path const& script_file,
                                   std::filesystem::path const& patch_folder) {
  CHECK_PATCH_FOLDER(patch_folder)
  imas::file::OperationScenario scenario;
  if(auto res = validate(scenario, game_folder, script_file); !res.first){
    return res;
  }

}

std::pair<bool, std::string> generate(std::filesystem::path const& game_folder, std::filesystem::path const& script_file,
                                      std::filesystem::path const& patch_folder) {
  CHECK_PATCH_FOLDER(patch_folder)

}

int main(int argc, char const *argv[])
{
    if (argc < 4) {
        std::cout << help << std::endl;
        return 1;
    }
    //check if directory exists
    if (!std::filesystem::is_directory(argv[2])) {
        std::cout << "Directory " << argv[2] << " does not exist" << std::endl;
        return 1;
    }
    //check if script exists
    if (!std::filesystem::exists(argv[3])) {
        std::cout << "Script file " << argv[3] << " does not exist" << std::endl;
        return 1;
    }
    switch (argv[1][0]) {
        case 'e': {
            return printResult(extract(argv[2], argv[3], argv[4]));
        }
        case 'p': {
            return printResult(patch(argv[2], argv[3], argv[4]));
        }
        case 'g': {
            return printResult(generate(argv[2], argv[3], argv[4]));
        }
        case 'v': {
            return printResult(validate(argv[2], argv[3]));
        }
        default: {
            std::cout << help << std::endl;
            return 1;
        }
    }

    return 0;
}
