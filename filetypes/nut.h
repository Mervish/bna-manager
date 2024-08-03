#pragma once

#include <boost/json.hpp>
#include <filesystem>
#include <string>
#include <vector>


namespace imas {
namespace file {

struct TextureData {
  //	int texture_data_size;		// size including header
  int unknown0;
  //	int image_data_size;		// size except for header
  //	int header_size;
  int unknown1;
  int nMipmap;
  int pixel_type;

  int width;
  int height;

  int unknown2;
  int unknown3;
  int unknown4;
  int unknown5;
  int unknown6;
  int unknown7;

  std::vector<int> mipmap_size;

  // eXt
  struct {
    int param1;
    int param2;
    int param3;
  }ext;

  // GIDX
  struct {
  int unknown11;
  int GIDX;
  int unknown12;
  }gidx;

  // raw data of image
  std::vector<char> raw_texture;

  bool Load(std::ifstream &stream);
  void Write(std::ofstream& stream);

  boost::json::value toJson() const;
  std::pair<bool, std::string> fromJson(boost::json::value const& value);
  //TO DO: Maybe add size validation
  std::pair<bool, std::string> exportDDS(std::filesystem::path const& extract_dir_path) const;
  std::pair<bool, std::string> importDDS(std::filesystem::path const& filepath);
};

struct NUT {
public:
  std::pair<bool, std::string> LoadNUT(std::filesystem::path const &filepath);
  std::pair<bool, std::string> SaveNUT(std::filesystem::path const &filepath);
  std::pair<bool, std::string>
  ExportDDS(const std::filesystem::path &dirpath) const;
  std::pair<bool, std::string> ImportDDS(
      const std::filesystem::path &dirpath); // replaces textures in the file
  std::pair<bool, std::string>
  LoadDDS(const std::filesystem::path &dirpath); // builds nut from scratch
  void reset();

private:
  std::vector<TextureData> texture_data;

  int unknown0;
  int unknown1;
  int unknown2;
  int unknown3;
  int unknown4;
};

struct DDS_HEADER {
  int dwMagic = ' SDD'; // 'DDS '

  int dwSize;   // 構造体のサイズ。常に124
  int dwFlags;  // 有効なフィールドを示すフラグ
  int dwHeight; // イメージの高さ
  int dwWidth;  // イメージの幅
  int dwPitchOrLinearSize; // 圧縮フォーマットの場合はイメージの総バイト数
  int dwDepth;
  int dwMipMapCount;
  int dwReserved1[11];

  struct DDPIXELFORMAT {
    int dwSize;
    int dwFlags;
    int dwFourCC;
    int dwRGBBitCount;
    int dwRBitMask;
    int dwGBitMask;
    int dwBBitMask;
    int dwRGBAlphaBitMask;
  } ddpfPixelFormat;

  struct DDCAPS2 {
    int dwCaps1;
    int dwCaps2;
    int Reserved[2];
  } ddsCaps;

  int dwReserved2;
};

}; // namespace file
}; // namespace imas
