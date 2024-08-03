
#include <filesystem>
#include <iostream>

#include "filetypes/scb.h"

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
        std::cout << "Usage: " << argv[0] << " <*.scb/*.csv>" << std::endl;
        return 1;
    }

    auto path = std::filesystem::path(argv[1]);
    std::filesystem::path payload;
    if(3 == argc) {
      payload = std::filesystem::path(argv[2]);
    }

    //we can accept only .scb or .csv files
    if(!std::filesystem::is_regular_file(path)) {
            std::cout << path << " is not a file."  << std::endl;
    }
    auto const extension = path.extension();
    if(extension != ".scb" && extension != ".csv") {
        std::cout << path << " is not a SCB or CSV file." << std::endl;
        return 1;
    }

    imas::file::SCB scb;
    if(extension == ".scb") {
        scb.loadFromFile(path);
        path.replace_extension(".csv");
        if (rewriteCheck(path)) {
            scb.extract(path);
            std::cout << "Saved to " << path << std::endl;
        }
    } else {
        scb.loadFromFile(path);
        scb.inject(payload);
    }

    return 0;
}
