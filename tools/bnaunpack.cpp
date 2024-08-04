#include "filetypes/bna.h"

#include <iostream>

constexpr auto help_text = "BNA (Un)Pack tool\n"
                           "To unpack a BNA file:\n"
                           "bnacmdtool <filename>\n"
                           "To pack a folder into a BNA file:\n"
                           "bnacmdtool <directory> or bnacmdtool <directory> <filename>";

void unpackFile(std::filesystem::path const& filepath, std::filesystem::path const& dirpath)
{
  imas::file::BNA bna;
  bna.loadFromFile(filepath);
  //check if folder exists
  if (std::filesystem::exists(dirpath)) {
    //prompt user
    std::cout << "Directory " << dirpath << " already exists. Overwrite? (y/n) ";
    std::string answer;
    std::getline(std::cin, answer);
    if (answer == "y"){
      std::filesystem::remove_all(dirpath);
    } else {
      return;
    }
  }
  bna.extractAllToDir(dirpath);
}

void unpackFile(std::filesystem::path const& filepath)
{
  auto const final_dir = filepath / filepath.stem();
  unpackFile(filepath, final_dir);
}

void packDir(std::filesystem::path const& dirpath) {

}

void packDir(std::filesystem::path const& dirpath, std::filesystem::path const& filepath) {

}

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cout << help_text;
    std::string answer;
    std::getline(std::cin, answer);
    return 1;
  }
  switch (argc)
  {
  case 2:
    [[fallthrough]];
  case 3:
  {
    auto path = argv[1];
    if(std::filesystem::is_regular_file(path))
    {
      if(argc == 3)
      {
        auto const dir_path = argv[2];
        unpackFile(path, dir_path);
      }
      unpackFile(path);
      return 0;
    }
    if(std::filesystem::is_directory(path))
    {
      //check if we have a third argument
      if(argc == 3)
      {
        //validate pack path
        auto pack_path = argv[2];
        if(std::filesystem::is_regular_file(pack_path))
        {
          packDir(path, pack_path);
        }else{
          std::cout << "Invalid pack path" << std::endl;
        }
      }
      packDir(path);
      return 0;
    }

    //bna::unpack(argv[1], argv[2]);
  }
    break;
  default:
    std::cout << help_text;
    return 1;
  }

  return 0;
}
