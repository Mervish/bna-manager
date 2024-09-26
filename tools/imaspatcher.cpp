
#include <format>
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

#define CREATE_PATCH_FOLDER(patch_folder) \
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
"Usage: imaspatcher <command> <game folder> <patch folder> <script file> <output script>\n"
"Commands:\n"
"  extract - exports game files to the patch folder\n"
"  patch - patches game files with the files from the patch folder\n"
"  unpack - unpacks game files 'as is', without conversion\n"
"  replace - replaces files without conversion\n"
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

struct TaskData {
  std::filesystem::path game;
  std::filesystem::path patch;
  std::filesystem::path script;
  std::filesystem::path out_script;
};

imas::file::Result iterateBNA(TaskData const& task,
                              std::string const& print,
                              auto &&pred) {
  imas::file::OperationScenario scenario;
  if (auto const res = scenario.fromFile(task.script); !res.first) {
    return res;
  }
  for (auto const &entry : scenario.entries) {
    std::cout << "Working with archive " << entry.path.string() << ":\n";
    auto const original_path = task.game / entry.path;
    imas::file::BNA bna;
    PRINT_ERROR_AND_CONTINUE(bna.loadFromFile(original_path))
    auto const &file_data = bna.getFileData();
    for (auto const &subentry : entry.files) {
      auto const it = std::ranges::find_if(file_data,
                                   [subentry](auto const &file) {
                                     return subentry == file.getFullPath();
                                   });
      if (it == file_data.end()) {
        std::cout << std::format("Failed to find BNA entry {}", subentry.string());
        continue;
      }
      auto const signature = it->getSignature();
      auto final_path = task.patch / subentry;
      if(auto const res = pred(subentry, bna, signature, final_path); res.first) {
        std::cout << print << final_path << '\n';
      }else{
        std::cout << MAKE_ERROR(res.second);
      }
    }
  }
  return {true, ""};
}

imas::file::Result extract(TaskData const& task) {
  // create the patch folder if it doesn't exist
  if (!std::filesystem::exists(task.patch)) {
    std::filesystem::create_directories(task.patch);
  }
  imas::file::OperationScenario scenario;
  if(auto const res = scenario.fromFile(task.script); !res.first) {
    return res;
  }
  for(auto const& entry: scenario.entries) {
    auto const original_path = task.game / entry.path;
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
      auto final_path = task.patch / subentry;
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

imas::file::Result unpack(TaskData const& task) {
  // create the patch folder if it doesn't exist
  if (!std::filesystem::exists(task.patch)) {
    std::filesystem::create_directories(task.patch);
  }
  auto const res = iterateBNA(task, "\tExtracted ",
      [](std::filesystem::path const &subentry,
                      imas::file::BNA &bna,
                      imas::file::BNAFileSignature const &signature,
                      std::filesystem::path &final_path) {
        std::filesystem::create_directories(final_path.parent_path());
        return bna.extractFile(signature, final_path);
      });
  return res.first ? imas::file::Result{true, {"Files extracted."}} : res;
}

imas::file::Result replace(TaskData const& task) {
  CHECK_PATCH_FOLDER(task.patch)
  auto const res = iterateBNA(task, "\tReplaced ",
      [](std::filesystem::path const &subentry,
                      imas::file::BNA &bna,
                      imas::file::BNAFileSignature const &signature,
                      std::filesystem::path &final_path) {
        return bna.replaceFile(signature, final_path);
      });
  return res.first ? imas::file::Result{true, {"Files replaced."}} : res;
}

imas::file::Result patch(TaskData const& task) {
  CHECK_PATCH_FOLDER(task.patch)
  std::cout << "Patching folder at: " << task.game.string() << '\n';
  imas::file::OperationScenario scenario;
  if(auto const res = scenario.fromFile(task.script); !res.first) {
    return res;
  }
  for(auto const& entry: scenario.entries) {
    std::cout << "Working with archive " << entry.path.string() << ":\n";
    auto const original_path = task.game / entry.path;
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
      auto final_path = task.patch / subentry;
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


imas::file::Result validate(TaskData const& task) {
  CHECK_PATCH_FOLDER(task.patch)
  imas::file::OperationScenario scenario;
  imas::file::OperationScenario output_scenario;
  if(auto const res = scenario.fromFile(task.script); !res.first) {
    return res;
  }
  for(auto const& entry: scenario.entries) {
    imas::file::OperationEntry candidate{.path = entry.path};
    auto const original_path = task.game / entry.path;
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
      auto final_path = task.patch / subentry;
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
    std::ofstream out{task.out_script};
    out << value;
    out.close();
    return {true, {"Script validated. New script created at " + task.out_script.string()}};
  }
  return {false, {"Script validated. No changes."}};
}


int main(int argc, char const *argv[])
{
    if (argc < 5) {
        std::cout << help;
        std::string answer;
        std::getline(std::cin, answer);
        return 0;
    }
    TaskData task;
    task.game = argv[2];
    task.script = argv[3];
    task.patch = argv[4];

    //check if directory exists
    if (!std::filesystem::is_directory(task.game)) {
        std::cout << "Directory " << task.game << " does not exist\n";
        return 1;
    }
    //check if script exists
    if (!std::filesystem::exists(task.script)) {
        std::cout << "Script file " << task.script << " does not exist\n";
        return 1;
    }
    switch (argv[1][0]) {
        case 'e': {
            return printResult(extract(task));
        }
        case 'p': {
            return printResult(patch(task));
        }
        case 'u': {
            return printResult(unpack(task));
        }
        case 'r': {
            return printResult(replace(task));
        }
        case 'v': {
            if(argc < 6) {
                std::cout << "Not enough arguments\n";
                std::cout << help;
                return 1;
            }
            task.out_script = argv[5];
            return printResult(validate(task));
        }
        default: {
            std::cout << help;
            return 1;
        }
    }
    return 0;
}
