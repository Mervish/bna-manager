
#include <QCoreApplication>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <filesystem>
#include <iostream>
#include <ranges>
#include <fstream>

#include "filetypes/bna.h"
#include "filetypes/scb.h"
#include "filetypes/msg.h"

//#define MAP_STRINGS

template<typename _Pred>
void iterateBNA(std::filesystem::path const& gamepath,
    _Pred const& callback) {
  auto const iter = std::filesystem::recursive_directory_iterator(gamepath);
  for (auto const& file :
       iter
           | std::views::filter(
               [](std::filesystem::directory_entry const& dir) {
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

void getBNAStruct(std::filesystem::path const& gamepath) {
    //full filesystem
    std::ofstream filesystem_output;
    filesystem_output.open("filesystem.txt", std::ios_base::trunc);
    //only script(*.scb) files
    std::ofstream script_output;
    script_output.open("script_fs.txt", std::ios_base::trunc);
    auto const printBNA = [&gamepath, &filesystem_output, &script_output](std::filesystem::path const& filepath) {
        imas::file::BNA bna;
        bna.loadFromFile(filepath);
        auto const rel = std::filesystem::relative(filepath, gamepath);
        filesystem_output << '[' << rel << ']' << std::endl;
        auto const& filedata = bna.getFileData();
        if(filedata.empty()){
            filesystem_output << "-(empty)" << std::endl;
        }
        for(auto const& subfile : filedata) {
            filesystem_output << '-' << subfile.dir_name << '/' << subfile.file_name << std::endl;
        }
        auto const& script_files = bna.getFiles("scb");
        if(!script_files.empty()) {
            script_output << '[' << rel << ']' << std::endl;
            for(auto const& scriptfile: script_files) {
                script_output << '*' << scriptfile.get().dir_name << '/' << scriptfile.get().file_name << std::endl;
            }
        }
    };
    iterateBNA(gamepath, printBNA);
}

bool compareFiles(std::filesystem::path const& p1, std::filesystem::path const& p2) {
  std::ifstream f1(p1, std::ifstream::binary|std::ifstream::ate);
  std::ifstream f2(p2, std::ifstream::binary|std::ifstream::ate);

  if (f1.fail() || f2.fail()) {
    return false; //file problem
  }

  if (f1.tellg() != f2.tellg()) {
    return false; //size mismatch
  }

  //seek back to beginning and use std::equal to compare contents
  f1.seekg(0, std::ifstream::beg);
  f2.seekg(0, std::ifstream::beg);
  return std::equal(std::istreambuf_iterator<char>(f1.rdbuf()),
                    std::istreambuf_iterator<char>(),
                    std::istreambuf_iterator<char>(f2.rdbuf()));
}

void rebuildAllBNA(std::filesystem::path const& gamepath) {
    auto const backup_path = gamepath.string() + "_rebuilt";
    std::ofstream report_output;
    report_output.open("rebuild_report.txt", std::ios_base::trunc);
    auto const rebuildBNA = [&backup_path, &gamepath, &report_output](std::filesystem::path const& filepath) {
      report_output << "Rebuilding " << filepath << '\t';
      auto const bna_rel_path = std::filesystem::relative(filepath, gamepath);
      auto const bna_save_path = backup_path / bna_rel_path;
      imas::file::BNA bna;
      bna.loadFromFile(filepath);
      auto const dirPath = std::filesystem::path(bna_save_path).parent_path();
      std::filesystem::create_directories(dirPath);
      bna.saveToFile(bna_save_path);
      report_output << "Rebuilt " << filepath << '\t';
      if(compareFiles(filepath, bna_save_path)) {
        report_output << "SAME" << std::endl;  
      }else{
        report_output << "DIFFERENT" << std::endl;
      }
    };
    iterateBNA(gamepath, rebuildBNA);
}

void getMasterScript(std::filesystem::path const& gamepath) {
#ifdef MAP_STRINGS
  size_t line_counter = 0;
#endif
  QJsonArray master_json;
#ifdef MAP_STRINGS
    std::function<void(std::filesystem::path const&)> bnaInjection = [&gamepath, &line_counter, &master_json](std::filesystem::path const& filepath) {
#else
    auto const bnaInjection = [&gamepath, &master_json](std::filesystem::path const& filepath) {
#endif
    imas::file::BNA bna;
    bna.loadFromFile(filepath);
    auto const rel = std::filesystem::relative(filepath, gamepath).string();
    auto const& script_files = bna.getFiles("scb");
    if(script_files.empty()) {
      return;
    }
    QJsonArray scb_json;
    for(auto const& scriptfile: script_files) {
      QJsonObject scb_entry;
      scb_entry["name"] = QString::fromStdString(std::string(scriptfile.get().dir_name) + '/' + scriptfile.get().file_name);
      imas::file::SCB scb;
      scb.loadFromData(scriptfile.get().file_data);
#ifndef MAP_STRINGS
      scb_entry["strings"] = scb.msg_data().getJson();
#else
      auto strings_json = scb.msg_data().getJson();
      for(auto line: strings_json) {
        line = QString("STRING #%1").arg(line_counter++, 5, 10, QChar('0'));
      }
      scb_entry["strings"] = strings_json;
#endif
      scb_json.append(scb_entry);
    }
    QJsonObject bna_json;
    bna_json["name"] = rel.c_str();
    bna_json["scb"] = scb_json;

    master_json.append(bna_json);
  };
  iterateBNA(gamepath, bnaInjection);
  QFile scriptfile("master_script.json");
  scriptfile.open(QFile::WriteOnly);
  scriptfile.write(QJsonDocument(master_json).toJson());
}

void setMasterScript(std::filesystem::path const& gamepath, std::string const& scriptpath) {
    auto const backup_path = std::filesystem::path(gamepath.string() + "_backup");
    QFile scriptfile(QString::fromStdString(scriptpath));
    scriptfile.open(QFile::ReadOnly);
    auto const script_array = QJsonDocument::fromJson(scriptfile.readAll()).array();
    for(auto const& entry: script_array) {
        auto const entry_obj = entry.toObject();
        auto const bna_path = entry_obj["name"].toString().toStdString();
        auto const bna_rel_path = gamepath / bna_path;
        auto const bna_save_path = backup_path / bna_path;
        imas::file::BNA bna;
        bna.loadFromFile(bna_rel_path);
        auto const& script_files = bna.getFiles("scb");
        for(auto const& scb_json: entry_obj["scb"].toArray()){
            auto const scb_obj = scb_json.toObject();
            auto const path = scb_obj["name"].toString().toStdString();
            auto const strings_json = scb_obj["strings"].toArray();
            auto res = std::ranges::find_if(script_files, [&path](std::reference_wrapper<imas::file::BNAFileEntry> const& file){
                return path == std::string(file.get().dir_name) + '/' + file.get().file_name;
            });
            if(res != script_files.end()) {
                imas::file::SCB scb;
                scb.loadFromData(res->get().file_data);
                scb.msg_data().setJson(strings_json);
                scb.rebuild();
                scb.saveToData(res->get().file_data);
            }
        }
        auto const dirPath = std::filesystem::path(bna_save_path).parent_path();
        std::filesystem::create_directories(dirPath);
        bna.saveToFile(bna_save_path);
    }
}

int main(int argc, char *argv[])
{
    switch (argc) {
    case 2:
    {
        auto const game_path = argv[1];
        if (!std::filesystem::is_directory(game_path)) {
            std::cout << "Not a directory: " << game_path << std::endl;
            return -1;
        }
        //getMasterScript(game_path);
        rebuildAllBNA(game_path);
    }
        break;
    /*case 3:
    {
        auto const game_path = argv[1];
        std::string const script_path = argv[2];
        if(!std::filesystem::is_directory(game_path)) {
            std::cout << "Not a directory: " << game_path << std::endl;
            return -1;
        }
        if(!std::filesystem::is_regular_file(script_path)) {
            std::cout << "Not a file: " << script_path << std::endl;
        }
        if(script_path.substr(script_path.size() - 5, 5) != ".json") {
            std::cout << "Not a JSON file: " << script_path << std::endl;
        }
        setMasterScript(argv[1], argv[2]);
        break;
    }*/
    default:
        std::cout << "Usage: BNAMaster <filename>" << std::endl;
        break;
    }

    return 0;
}
