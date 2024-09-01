
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
  return {false, "file does not exist."}; \
}

#define MAKE_ERROR(s) \
  std::string{"error, "} + s + "\n"

// check if file exists and skip
#define CHECK_FILE_SKIP(file) \
if (!std::filesystem::exists(file)) { \
  std::cout << MAKE_ERROR("patch file missing (expected - " + file.string() + ")"); \
  continue; \
}

#define PRINT_ERROR_AND_CONTINUE(func) \
if(auto const ret = func; !ret.first) { \
  std::cout << MAKE_ERROR(ret.second); \
  continue; \
}

#define PRINT_RES(func) \
if(auto const ret = func; ret.first) { \
  std::cout << "success\n"; \
}else{ \
  std::cout << MAKE_ERROR(ret.second); \
  continue; \
}

constexpr auto help = "Idolm@ster patching tool.\n"
"Usage: imaspatcher <command> <game folder> <script file> <patch folder> <output script>\n"
"Commands:\n"
"  extract - exports game files to the patch folder\n"
"  patch - patches game files with the files from the patch folder\n"
"  validate - validate and clean the script file from unused entries\n";

template <class T>
void replaceExtension(std::filesystem::path& path, T const& filetype) {
  auto const ext = filetype.api().final_extension;
  path.replace_extension(ext);
}

void makeDirs(std::filesystem::path const& path) {
  if (!std::filesystem::exists(path)) {
    std::filesystem::create_directories(path);
  }
}

imas::file::Result extract(std::filesystem::path const& game_folder, std::filesystem::path const& script_file,
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
    PRINT_ERROR_AND_CONTINUE(bna.loadFromFile(original_path))
    auto const& file_data = bna.getFileData();
    for(auto const& subentry: entry.files) {
      auto const it = std::find_if(file_data.begin(), file_data.end(), [subentry](auto const& file) {
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
          PRINT_ERROR_AND_CONTINUE(bxr.loadFromData(file.file_data))
          PRINT_ERROR_AND_CONTINUE(bxr.extract(final_path))
          std::cout << "Extracted " << final_path << '\n';
        }
        break;
        case imas::filetype::type::nut:
        {
          imas::file::NUT nut;
          final_path.replace_extension();
          std::filesystem::create_directories(final_path.parent_path());
          PRINT_ERROR_AND_CONTINUE(nut.loadFromData(file.file_data))
          PRINT_ERROR_AND_CONTINUE(nut.extract(final_path))
          std::cout << "Extracted " << final_path << '\n';
        }
        break;
        case imas::filetype::type::scb:
        {
          imas::file::SCB scb;
          replaceExtension(final_path, scb);
          makeDirs(final_path.parent_path());
          PRINT_ERROR_AND_CONTINUE(scb.loadFromData(file.file_data))
          PRINT_ERROR_AND_CONTINUE(scb.extract(final_path))
          std::cout << "Extracted " << final_path << '\n';
        }
        break;
        default:
        return {false, "Unsupported file type."};
      }
    }
  }
  return {true, {"Data extracted."}};
}

imas::file::Result patch(std::filesystem::path const& game_folder, std::filesystem::path const& script_file,
                                   std::filesystem::path const& patch_folder) {
  CHECK_PATCH_FOLDER(patch_folder)
  std::cout << "Patching folder at: " << game_folder.string() << '\n';
  imas::file::OperationScenario scenario;
  if(auto const res = scenario.fromFile(script_file); !res.first) {
    return res;
  }
  for(auto const& entry: scenario.entries) {
    std::cout << "Working with archive " << entry.path.string() << ":\n";
    auto const original_path = game_folder / entry.path;
    imas::file::BNA bna;
    if(auto const ret = bna.loadFromFile(original_path); !ret.first) {
      std::cout << ret.second;
      continue;
    }
    auto const& file_data = bna.getFileData();
    bool changed = false;
    for(auto const& subentry: entry.files) {
      std::cout << "\ttrying to patch file " << subentry.string() << ": ";
      auto const it = std::find_if(file_data.begin(), file_data.end(), [subentry](auto const& file){
              return subentry == file.getFullPath();
            });
      if (it == file_data.end()) {
        std::cout << MAKE_ERROR("failed to find");
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
          PRINT_ERROR_AND_CONTINUE(bxr.inject(final_path));
          PRINT_RES(bxr.saveToData(file.file_data));
        }
        break;
        case imas::filetype::type::nut:
        {
          imas::file::NUT nut;
          final_path.replace_extension();
          nut.loadFromData(file.file_data);
          PRINT_ERROR_AND_CONTINUE(nut.inject(final_path));
          PRINT_RES(nut.saveToData(file.file_data));
        }
        break;
        case imas::filetype::type::scb:
        {
          imas::file::SCB scb;
          replaceExtension(final_path, scb);
          CHECK_FILE_SKIP(final_path)
          scb.loadFromData(file.file_data);
          PRINT_ERROR_AND_CONTINUE(scb.inject(final_path));
          PRINT_RES(scb.saveToData(file.file_data));
        }
        break;
        default:
        {
          std::cout << MAKE_ERROR("unsupported file type");
          continue;
        }
      }
      changed = true;
    }
    if(changed) {
      bna.saveToFile(original_path);
      std::cout << "Patched\n";
    }else{
      std::cout << "Skipped\n";
    }
  }
  return {true, {"Patch applied."}};
}


imas::file::Result validate(std::filesystem::path const& game_folder, std::filesystem::path const& script_file,
                            std::filesystem::path const& patch_folder, std::filesystem::path const& output_script) {
  CHECK_PATCH_FOLDER(patch_folder)
  imas::file::OperationScenario scenario;
  imas::file::OperationScenario output_scenario;
  if(auto const res = scenario.fromFile(script_file); !res.first) {
    return res;
  }
  for(auto const& entry: scenario.entries) {
    imas::file::OperationEntry candidate{.path = entry.path};
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
          final_path.replace_extension("xml");
          if(std::filesystem::exists(final_path)) {
            candidate.files.push_back(subentry);
          }
        }
        break;
        case imas::filetype::type::nut:
        {
          imas::file::NUT nut;
          final_path.replace_extension();
          nut.loadFromData(file.file_data);
          if(nut.hasFiles(final_path)) {
            candidate.files.push_back(subentry);
          }
        }
        break;
        case imas::filetype::type::scb:
        {
          final_path.replace_extension("csv");
          if(std::filesystem::exists(final_path)) {
            candidate.files.push_back(subentry);
          }
        }
        break;
        default:
        std::cout << MAKE_ERROR("unsupported file type");
      }
    }
    if(!candidate.files.empty()) {
      output_scenario.entries.push_back(candidate);
    }
  }
  if(!output_scenario.entries.empty()) {
    auto const value = output_scenario.toJSON();
    std::ofstream out{output_script};
    out << value;
    out.close();
    return {true, {"Script validated. New script created at " + output_script.string()}};
  }
  return {false, {"Script validated. No changes."}};
}


int main(int argc, char const *argv[])
{
    if (argc < 4) {
        std::cout << help;
        std::string answer;
        std::getline(std::cin, answer);
        return 0;
    }
    //check if directory exists
    if (!std::filesystem::is_directory(argv[2])) {
        std::cout << "Directory " << argv[2] << " does not exist\n";
        return 1;
    }
    //check if script exists
    if (!std::filesystem::exists(argv[3])) {
        std::cout << "Script file " << argv[3] << " does not exist\n";
        return 1;
    }
    switch (argv[1][0]) {
        case 'e': {
            return printResult(extract(argv[2], argv[3], argv[4]));
        }
        case 'p': {
            return printResult(patch(argv[2], argv[3], argv[4]));
        }
        case 'v': {
            if(argc < 6) {
                std::cout << "Not enough arguments\n";
                std::cout << help;
                return 1;
            }
            return printResult(validate(argv[2], argv[3], argv[4], argv[5]));
        }
        default: {
            std::cout << help;
            return 1;
        }
    }
    return 0;
}
