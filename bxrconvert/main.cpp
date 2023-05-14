
#include <filesystem>
#include <iostream>

#include "filetypes/bxr.h"

bool rewriteCheck(std::filesystem::path const& filepath) {
    if(std::filesystem::exists(filepath)) {
        std::cout << "File " << filepath << " already exists. Overwrite? [y/N] ";
        char c;
        std::cin >> c;
        return c == 'y';
    }
    return true;
}

int main(int argc, char *argv[])
{
    if(argc < 2) {
        std::cout << "Usage: " << argv[0] << " <*.brx/*.xml>" << std::endl;
        return 1;
    }

    auto path = std::filesystem::path(argv[1]);
    //we can accept only .bxr or .xml files
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
        bxr.load(path);
        path.replace_extension(".xml");
        if (rewriteCheck(path)) {
            bxr.writeXML(path);
            std::cout << "Saved to " << path << std::endl;
        }
    } else {
        bxr.readXML(path);
        path.replace_extension(".bxr");
        if (rewriteCheck(path)) {
            bxr.save(path);
            std::cout << "Saved to " << path << std::endl;
        }
    }

    return 0;
}
