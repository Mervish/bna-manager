
#include <iostream>

#include "filetypes/bna.h"
#include "filetypes/bxr.h"
#include "filetypes/nut.h"
#include "filetypes/scb.h"
#include "filetypes/scenario.h"
#include "utility/commandline.h"
#include "utility/filetype.h"


#include <boost/json.hpp>

// check if the patch folder exists
#define CHECK_PATCH_FOLDER(patch_folder) \
if (!std::filesystem::exists(patch_folder)) { \
  return {false, "Patch folder does not exist."}; \
}

// check if file exists
#define CHECK_FILE(file) \
if (!std::filesystem::exists(file)) { \
  return {false, "File does not exist."}; \
}

// check if file exists and skip
#define CHECK_FILE_SKIP(file) \
if (!std::filesystem::exists(file)) { \
  continue; \
}

constexpr auto help = "Idolm@ster patching tool.\n"
"Usage: imaspatcher <command> <game folder> <script file> <patch folder>\n"
"Commands:\n"
"  extract - exports game files to the patch folder\n"
"  patch - patches game files with the files from the patch folder\n"
/*"  generate - generates a script file from the patch folder\n"
"  validate - validates the script file"*/;

template <class T>
void replaceExtension(std::filesystem::path& path, T const& filetype) {
  auto const ext = filetype.getApi().final_extension;
  path.replace_extension(ext);
}

void makeDirs(std::filesystem::path const& path) {
  if (!std::filesystem::exists(path)) {
    std::filesystem::create_directories(path);
  }
}

std::pair<bool, std::string> extract(std::filesystem::path const& game_folder, std::filesystem::path const& script_file,
                                     std::filesystem::path const& patch_folder) {
  // create the patch folder if it doesn't exist
  if (!std::filesystem::exists(patch_folder)) {
    std::filesystem::create_directories(patch_folder);
  }
  imas::file::OperationScenario scenario;
  if(auto const res = scenario.fromFile(script_file); !res.first) {
    return res;
  }
  for(auto const& entry: scenario.entries) {
    auto const original_path = game_folder / entry.path;
    imas::file::BNA bna;
    if(auto res = bna.loadFromFile(original_path); !res.first)
    {
       return res;
    }
    auto const& file_data = bna.getFileData();
    for(auto const& subentry: entry.files) {
      auto const it = std::find_if(file_data.begin(), file_data.end(), [subentry](auto const& file){
              return subentry == file.getFullPath();
            });
      if (it == file_data.end()) {
        continue;
      }
      auto const signature = it->getSignature();
      auto file = bna.getFile(signature);
      auto const ext_type = imas::filetype::getFileType(subentry);
      auto final_path = patch_folder / subentry;
      switch (ext_type) {
        case imas::filetype::type::bxr:
        {
          imas::file::BXR bxr;
          replaceExtension(final_path, bxr);
          makeDirs(final_path.parent_path());
          bxr.loadFromData(file.file_data);
          bxr.extract(final_path);
        }
        break;
        // case imas::filetype::type::nut:
        // {
        //   imas::file::NUT nut;
        //   final_path.replace_extension();
        //   nut.LoadNUT(subentry);
        //   nut.ExportDDS(final_path);
        // }
        // break;
        case imas::filetype::type::scb:
        {
          imas::file::SCB scb;
          replaceExtension(final_path, scb);
          makeDirs(final_path.parent_path());
          scb.loadFromData(file.file_data);
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
  if(auto const res = scenario.fromFile(script_file); !res.first) {
    return res;
  }
  for(auto const& entry: scenario.entries) {
    auto const original_path = game_folder / entry.path;
    imas::file::BNA bna;
    CONT_ON_ERROR(bna.loadFromFile(original_path))
    //STOP_ON_ERROR_RET(bna.loadFromFile(original_path))
    auto const& file_data = bna.getFileData();
    for(auto const& subentry: entry.files) {
      auto const it = std::find_if(file_data.begin(), file_data.end(), [subentry](auto const& file){
              return subentry == file.getFullPath();
            });
      if (it == file_data.end()) {
        continue;
      }
      auto const signature = it->getSignature();
      auto &file = bna.getFile(signature);
      auto const ext_type = imas::filetype::getFileType(subentry);
      auto final_path = patch_folder / subentry;
      switch (ext_type) {
        case imas::filetype::type::bxr:
        {
          imas::file::BXR bxr;
          replaceExtension(final_path, bxr);
          CHECK_FILE_SKIP(final_path)
          bxr.loadFromFile(final_path);
          bxr.saveToData(file.file_data);
        }
        break;
        //case imas::filetype::type::nut:
        case imas::filetype::type::scb:
        {
          imas::file::SCB scb;
          replaceExtension(final_path, scb);
          CHECK_FILE_SKIP(final_path)
          scb.loadFromData(file.file_data);
          STOP_ON_ERROR_RET(scb.inject(final_path));
          scb.saveToData(file.file_data);
        }
        break;
        default:
        return {false, "Unsupported file type."};
      }
    }
    bna.saveToFile(original_path);
    std::cout << "Patched file " << original_path << std::endl;
  }
  return {true, {"Patch applied"}};
}

std::pair<bool, std::string> generate(std::filesystem::path const& game_folder, std::filesystem::path const& script_file,
                                      std::filesystem::path const& patch_folder) {
  CHECK_PATCH_FOLDER(patch_folder)

}

int main(int argc, char const *argv[])
{
    if (argc < 4) {
        std::cout << help << std::endl;
        std::string answer;
        std::getline(std::cin, answer);
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
        // case 'g': {
        //     return printResult(generate(argv[2], argv[3], argv[4]));
        // }
        // case 'v': {
        //     return printResult(validate(argv[2], argv[3]));
        // }
        default: {
            std::cout << help << std::endl;
            return 1;
        }
    }

    return 0;
}
