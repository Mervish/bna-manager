
#include <filesystem>
#include <iostream>

#include <utility/path.h>

#include "filetypes/bxr.h"

void testBXR(std::filesystem::path const &path) {
  std::filesystem::path test_root = path.string() + "_test";
  imas::path::iterateFiles(
      path, ".bxr", [&path, &test_root](std::filesystem::path const &filepath) {
        auto const rel = std::filesystem::relative(filepath, path);
        std::filesystem::path xml_path = filepath;
        xml_path.replace_extension(".xml");
        std::filesystem::path test_path = test_root / rel;
        std::filesystem::create_directories(test_path.parent_path());
        {
          imas::file::BXR bxr;
          bxr.loadFromFile(filepath);
          bxr.extract(xml_path);
        }
        {
          imas::file::BXR bxr;
          bxr.inject(xml_path);
          bxr.saveToFile(test_path);
        }
      });
}

int main(int argc, char *argv[])
{
    if(argc < 2) {
        std::cout << "Usage: " << argv[0] << " <*.brx/*.xml>" << std::endl;
        std::string answer;
        std::getline(std::cin, answer);
        return 1;
    }

    auto path = std::filesystem::path(argv[1]);
    //we can accept only .bxr or .xml files
    if(std::filesystem::is_directory(path)) {
      testBXR(path);
      return 0;
    }
    if(!std::filesystem::is_regular_file(path)) {
            std::cout << path << " is not a file."  << std::endl;
    }
    auto const extension = path.extension();
    if(extension != ".bxr" && extension != ".xml") {
        std::cout << path << " is not a BXR or XML file." << std::endl;
        return 1;
    }

    imas::file::BXR bxr;
    if(extension == ".bxr") {
        bxr.loadFromFile(path);
        auto test_path = path;
        //test_path.replace_filename(test_path.filename().stem().string() + "_test" + path.extension().string());
        //bxr.saveToFile(test_path);
        path.replace_extension(".xml");
        if (imas::path::rewriteCheck(path)) {
            bxr.extract(path);
            std::cout << "Saved to " << path << std::endl;
        }
    } else {
        bxr.inject(path);
        path.replace_extension(".bxr");
        if (imas::path::rewriteCheck(path)) {
            bxr.saveToFile(path);
            std::cout << "Saved to " << path << std::endl;
        }
    }

    return 0;
}
