
#include <filetypes/nfh.h>
#include <utility/result.h>
#include <utility/path.h>

#include <filesystem>
#include <iostream>

constexpr auto help_text =
    "NFH conversion tool\n"
    "nfhtool <*.nfh/*.json> - unpack/repack font file";
constexpr auto nfh_ext = ".nfh";
constexpr auto json_ext = ".json";

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cout << help_text;
    std::string answer;
    std::getline(std::cin, answer);
    return 1;
  }

  auto path = std::filesystem::path(argv[1]);
  //we can accept only .nfh or .json files
  if(!std::filesystem::is_regular_file(path)) {
          std::cout << path << " is not a file."  << std::endl;
  }
  auto const extension = path.extension();
  if(extension != nfh_ext && extension != json_ext) {
      std::cout << path << " is not a NFH or JSON file." << std::endl;
      return 1;
  }

  imas::file::NFH nfh;
  if(extension == nfh_ext) {
      nfh.loadFromFile(path);
      path.replace_extension(json_ext);
      if (imas::path::rewriteCheck(path)) {
          nfh.extract(path);
          std::cout << "Saved to " << path << std::endl;
      }
  } else {
      nfh.inject(path);
      path.replace_extension(nfh_ext);
      if (imas::path::rewriteCheck(path)) {
          nfh.saveToFile(path);
          std::cout << "Saved to " << path << std::endl;
      }
  }

  return 0;
}
