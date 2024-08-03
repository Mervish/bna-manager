#pragma once

#include <filesystem>

namespace imas {
namespace filetype {
enum class type
{
  bna,
  scb,
  bxr,
  nut,
  other
};

inline type getFileType(std::filesystem::path const& filepath) {
  auto const ext = std::filesystem::path(filepath).extension().string();
  if (ext == ".bna") {
    return type::bna;
  }
  if (ext == ".scb") {
    return type::scb;
  }
  if (ext == ".bxr") {
    return type::bxr;
  }
  if (ext == ".nut") {
    return type::nut;
  }
  return type::other;
}



}
}

