#pragma once

#include "utility/result.h"

#include <iostream>

#define STOP_ON_ERROR(res) if(!printResultOnError(res)) { return; }
#define STOP_ON_ERROR_RET(res) if(!printResultOnError(res)) { return res; }
#define CONT_ON_ERROR_PRINT(res) if(!printResultOnError(res)) { continue; }
#define CONT_ON_ERROR(res) if(!res.first) { continue; }

inline bool printResultOnError(imas::file::Result const& result) {
  if(!result.first) {
    std::cout << result.second << std::endl;
  }
  return result.first;
}

inline bool printResult(imas::file::Result const& result) {
  std::cout << result.second << std::endl;
  return result.first;
}
