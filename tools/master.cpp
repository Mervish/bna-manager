
#include "utility/path.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>

#include <filetypes/bna.h>
#include <filetypes/msg.h>
#include <filetypes/nut.h>
#include <filetypes/scb.h>
#include <filetypes/scenario.h>

#include <boost/algorithm/string.hpp>

// #define MAP_STRINGS

constexpr auto help =
    "Idolm@ster BNA charting tool.\n"
    "Usage: BNAMaster map [extension] <gamepath> <scenario_path>\n"
    "maps bna's internal files according to [extension] and writes them into "
    "the <scenario_patch>";

template <typename _Pred>
void iterateBNA(std::filesystem::path const &gamepath, _Pred const &callback) {
  auto const iter = std::filesystem::recursive_directory_iterator(gamepath);
  for (auto const &file :
       iter |
           std::views::filter([](std::filesystem::directory_entry const &dir) {
             if (dir.is_regular_file()) {
               // check is extension is BNA
               return dir.path().extension() == ".bna";
             } else {
               return false;
             }
           })) {
    callback(file.path());
  }
}

// void getBNAStruct(std::filesystem::path const &gamepath) {
//   // full filesystem
//   std::ofstream filesystem_output;
//   filesystem_output.open("filesystem.txt", std::ios_base::trunc);
//   // only script(*.scb) files
//   std::ofstream script_output;
//   script_output.open("script_fs.txt", std::ios_base::trunc);
//   auto const printBNA = [&gamepath, &filesystem_output, &script_output](
//                             std::filesystem::path const &filepath) {
//     imas::file::BNA bna;
//     bna.loadFromFile(filepath);
//     auto const rel = std::filesystem::relative(filepath, gamepath);
//     filesystem_output << '[' << rel << ']' << std::endl;
//     auto const &filedata = bna.getFileData();
//     if (filedata.empty()) {
//       filesystem_output << "-(empty)" << std::endl;
//     }
//     for (auto const &subfile : filedata) {
//       filesystem_output << '-' << subfile.dir_name << '/' <<
//       subfile.file_name
//                         << std::endl;
//     }
//     auto const &script_files = bna.getFiles("scb");
//     if (!script_files.empty()) {
//       script_output << '[' << rel << ']' << std::endl;
//       for (auto const &scriptfile : script_files) {
//         script_output << '*' << scriptfile.get().dir_name << '/'
//                       << scriptfile.get().file_name << std::endl;
//       }
//     }
//   };
//   iterateBNA(gamepath, printBNA);
// }

void mapBNA(std::filesystem::path const &gamepath, std::string const &filetype,
            std::filesystem::path const &scenario_path) {
  std::vector<std::string> filetypes;
  boost::split(filetypes, filetype, boost::is_any_of("|"));

  imas::file::OperationScenario scenario;
  auto const mapBNAfile = [&gamepath, &filetype, &scenario,
                           &filetypes](std::filesystem::path const &filepath) {
    imas::file::BNA bna;
    if (auto const res = bna.loadFromFile(filepath); !res.first) {
      std::cout << "Failed to open " << filepath << " - " << res.second
                << std::endl;
    }
    std::vector<std::reference_wrapper<imas::file::BNAFileEntry>> files;
    for (auto const &filetype : filetypes) {
      auto const f_files = bna.getFiles(filetype);
      files.insert(files.end(), f_files.cbegin(), f_files.cend());
    }
    if (files.empty()) {
      return;
    }
    auto const bna_rel_path = std::filesystem::relative(filepath, gamepath);
    imas::file::OperationEntry op_entry{.path = bna_rel_path};
    for (auto const &file : files) {
      op_entry.files.push_back(file.get().getFullPath());
    }
    scenario.entries.push_back(op_entry);
  };
  imas::path::iterateFiles(gamepath, ".bna", mapBNAfile);
  boost::json::value json = scenario.toJSON();
  std::ofstream output;
  output.open(scenario_path, std::ios_base::trunc);
  output << boost::json::serialize(json) << std::endl;
}

// bool compareFiles(std::filesystem::path const &p1,
//                   std::filesystem::path const &p2) {
//   std::ifstream f1(p1, std::ifstream::binary | std::ifstream::ate);
//   std::ifstream f2(p2, std::ifstream::binary | std::ifstream::ate);

//   if (f1.fail() || f2.fail()) {
//     return false; // file problem
//   }

//   if (f1.tellg() != f2.tellg()) {
//     return false; // size mismatch
//   }

//   // seek back to beginning and use std::equal to compare contents
//   f1.seekg(0, std::ifstream::beg);
//   f2.seekg(0, std::ifstream::beg);
//   return std::equal(std::istreambuf_iterator<char>(f1.rdbuf()),
//                     std::istreambuf_iterator<char>(),
//                     std::istreambuf_iterator<char>(f2.rdbuf()));
// }

// void rebuildAllBNA(std::filesystem::path const &gamepath) {
//   auto const backup_path = gamepath.string() + "_rebuilt";
//   std::ofstream report_output;
//   report_output.open("rebuild_report.txt", std::ios_base::trunc);
//   auto const rebuildBNA = [&backup_path, &gamepath, &report_output](
//                               std::filesystem::path const &filepath) {
//     report_output << "Rebuilding " << filepath << '\t';
//     auto const bna_rel_path = std::filesystem::relative(filepath, gamepath);
//     auto const bna_save_path = backup_path / bna_rel_path;
//     imas::file::BNA bna;
//     bna.loadFromFile(filepath);
//     auto const dirPath = std::filesystem::path(bna_save_path).parent_path();
//     std::filesystem::create_directories(dirPath);
//     bna.saveToFile(bna_save_path);
//     report_output << "Rebuilt " << filepath << '\t';
//     if (compareFiles(filepath, bna_save_path)) {
//       report_output << "SAME" << std::endl;
//     } else {
//       report_output << "DIFFERENT" << std::endl;
//     }
//   };
//   iterateBNA(gamepath, rebuildBNA);
// }

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << help << std::endl;
    std::string answer;
    std::getline(std::cin, answer);
    return -1;
  }

  auto const command = std::string(argv[1]);

  if (command == "map") {
    auto filetype = std::string(argv[2]);
    std::transform(filetype.begin(), filetype.end(), filetype.begin(),
                   ::tolower);
    auto const gamepath = std::string(argv[3]);
    auto const scenario_path = std::string(argv[4]);
    mapBNA(gamepath, filetype, scenario_path);
  }

  // if (command == "compare") {
  //   auto const nut_path = std::string(argv[2]);
  //   auto const bna_path = std::string(argv[3]);
  //   auto const nut_subpath = std::string(argv[4]);
  //   imas::file::BNA bna;
  //   bna.loadFromFile(bna_path);
  //   auto const& file_data = bna.getFileData();
  //   auto const it = std::find_if(file_data.begin(), file_data.end(),
  //   [nut_subpath](auto const& file){
  //             return nut_subpath == file.getFullPath();
  //           });
  //   if (it == file_data.end()) {
  //     return 1;
  //   }
  //   auto const signature = it->getSignature();
  //   auto file = bna.getFile(signature);
  //   imas::file::NUT sub_nut;
  //   sub_nut.loadFromData(file.file_data);
  //   imas::file::NUT nut;
  //   nut.loadFromFile(nut_path);
  //   auto const& texture1 = nut.texture_data.front().raw_texture;
  //   auto const& texture2 = sub_nut.texture_data.front().raw_texture;
  //   std::cout << (std::equal(texture1.begin(), texture1.end(),
  //   texture2.begin()) ? "SAME" : "DIFFERENT") << std::endl; auto const path =
  //   nut_path.substr(0, nut_path.find_last_of('.')) + "\\" + "original"; auto
  //   const sub_path = nut_path.substr(0, nut_path.find_last_of('.')) + "\\" +
  //   "sub"; std::filesystem::create_directories(path);
  //   std::filesystem::create_directories(sub_path);
  //   nut.extract(path);
  //   sub_nut.extract(sub_path);
  //   return 0;
  // }

  // switch (argc) {
  // case 2: {
  //   auto const game_path = argv[1];
  //   if (!std::filesystem::is_directory(game_path)) {
  //     std::cout << "Not a directory: " << game_path << std::endl;
  //     return -1;
  //   }
  //   // getMasterScript(game_path);
  //   rebuildAllBNA(game_path);
  // } break;
  // case 3: {
  //   auto const game_path = argv[1];
  //   std::string const script_path = argv[2];
  //   if (!std::filesystem::is_directory(game_path)) {
  //     std::cout << "Not a directory: " << game_path << std::endl;
  //     return -1;
  //   }
  //   if (!std::filesystem::is_regular_file(script_path)) {
  //     std::cout << "Not a file: " << script_path << std::endl;
  //   }
  //   if (script_path.substr(script_path.size() - 5, 5) != ".json") {
  //     std::cout << "Not a JSON file: " << script_path << std::endl;
  //   }
  //   setMasterScript(argv[1], argv[2]);
  //   break;
  // }
  // default:
  //   std::cout << "Usage: BNAMaster <filename>" << std::endl;
  //   break;
  // }

  return 0;
}
