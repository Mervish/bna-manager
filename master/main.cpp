
#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <filesystem>
#include <iostream>
#include <ranges>
#include <fstream>

#include "filetypes/bna.h"
#include "filetypes/scb.h"
#include "filetypes/msg.h"

/*void refactor() {
    auto iter = std::filesystem::recursive_directory_iterator(path);

    //full filesystem
    std::ofstream filesystem_output;
    filesystem_output.open("filesystem.txt", std::ios_base::trunc);
    //only script(*.scb) files
    std::ofstream script_output;
    script_output.open("script_fs.txt", std::ios_base::trunc);

    for (auto const &file : iter | std::views::filter([](std::filesystem::directory_entry const &dir) {
                                if(dir.is_regular_file()){
                                    //check is extension is BNA
                                    return dir.path().extension() == ".bna";
                                }else{
                                    return false;
                                }})) {
        std::cout << file.path().filename().string() << std::endl;
        imas::file::BNA bna;
        bna.loadFromFile(file.path().string());
        auto const rel = std::filesystem::relative(file.path(), path).string();
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
    }
}*/

void getMasterScript(std::string const& gamepath) {
    auto iter = std::filesystem::recursive_directory_iterator(gamepath);
    QJsonArray master_json;

    for (auto const &file : iter | std::views::filter([](std::filesystem::directory_entry const &dir) {
                                if(dir.is_regular_file()){
                                    //check is extension is BNA
                                    return dir.path().extension() == ".bna";
                                }else{
                                    return false;
                                }})) {
        imas::file::BNA bna;
        bna.loadFromFile(file.path().string());
        auto const rel = std::filesystem::relative(file.path(), gamepath).string();
        auto const& script_files = bna.getFiles("scb");
        if(script_files.empty()) {
            continue;
        }
        QJsonArray scb_json;
        for(auto const& scriptfile: script_files) {
            QJsonObject scb_entry;
            scb_entry["name"] = QString::fromStdString(std::string(scriptfile.get().dir_name) + '/' + scriptfile.get().file_name);
            imas::file::SCB scb;
            scb.loadFromData(scriptfile.get().file_data);
            scb_entry["strings"] = scb.msg_data().getJson();
            scb_json.append(scb_entry);
        }
        QJsonObject bna_json;
        bna_json["name"] = rel.c_str();
        bna_json["scb"] = scb_json;

        master_json.append(bna_json);
    }
    QFile scriptfile("master_script.json");
    scriptfile.open(QFile::WriteOnly);
    scriptfile.write(QJsonDocument(master_json).toJson());
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    if (argc < 2) {
        std::cout << "Usage: BNAMaster <filename>" << std::endl;
        return -1;
    }
    auto const path = argv[1];
    if (!std::filesystem::is_directory(path)) {
        std::cout << "Not a directory: " << path << std::endl;
        return -1;
    }
    getMasterScript(path);
    return 0;
    return a.exec();
}
