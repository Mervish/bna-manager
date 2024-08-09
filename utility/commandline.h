#pragma once

#include "utility/result.h"

#include <iostream>

#define STOP_ON_ERROR(res) if(!printResult(res)) { return; }
#define STOP_ON_ERROR_RET(res) if(!printResult(res)) { return res; }
#define CONT_ON_ERROR(res) if(!printResult(res)) { continue; }

inline bool printResult(imas::file::Result const& result) {
  if(!result.first) {
    std::cout << result.second << std::endl;
  }
  return result.first;
}
