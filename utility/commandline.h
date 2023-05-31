#pragma once

#include <iostream>

#define STOP_ON_ERROR(res) if(!printResult(res)) { return; }
#define CONT_ON_ERROR(res) if(!printResult(res)) { continue; }

inline bool printResult(std::pair<bool, std::string> const& result) {
  if(!result.first) {
    std::cout << result.second << std::endl;
  }
  return result.first;
}
