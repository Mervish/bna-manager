#pragma once

#include "filetypes/manageable.h"

#include <filesystem>
#include <vector>

#include <boost/json.hpp>

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

  std::filesystem::path const getFilePath(std::filesystem::path const& path) const;

  bool load(std::basic_istream<char> *stream);
  void write(std::basic_ostream<char> *stream);

  int16_t headerSize() const;
  int32_t calculateSize() const;

  boost::json::value toJson() const;
  Result fromJson(boost::json::value const& value);
  //TO DO: Maybe add size validation
  Result exportDDS(std::filesystem::path const& extract_dir_path) const;
  Result importDDS(std::filesystem::path const& filepath);
};

struct NUT : public Manageable {
public:
  Manageable::Fileapi api() const override;
  Result
  loadDDS(const std::filesystem::path &dirpath); // builds nut from scratch
  bool hasFiles(std::filesystem::path const& path) const;
  void reset();

  virtual Result extract(std::filesystem::path const& savepath) const override;
  virtual Result inject(std::filesystem::path const& openpath) override;

protected:
  Result openFromStream(std::basic_istream<char> *stream) override;
  Result saveToStream(std::basic_ostream<char> *stream) override;
  size_t size() const override;
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
