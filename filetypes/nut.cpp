#include "nut.h"

#include "nvtt/nvtt.h"
#include "utility/streamtools.h"

#include <fstream>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <utility/datatools.h>

namespace bjson = boost::json;

namespace {
void changeEndian2(std::vector<char> &target) {
  for (auto it = target.begin(); it != target.end(); it += 2) {
    std::iter_swap(it, it + 1);
  }
}

void changeEndian4(std::vector<char> &target) {
  for (auto it = target.begin(); it != target.end(); it += 4) {
    std::iter_swap(it, it + 3);
    std::iter_swap(it + 1, it + 2);
  }
}

}

namespace imas {
namespace file {

bool TextureData::load(std::basic_istream<char> *stream) {
  // texture header
  int texture_data_size = utility::readLong(stream);
  unknown0 = utility::readLong(stream); // always 0
  int image_data_size = utility::readLong(stream);
  int header_size = utility::readShort(stream);
  unknown1 = utility::readShort(stream); // always 0
  nMipmap = utility::readShort(stream);
  pixel_type = utility::readShort(stream);
  width = utility::readShort(stream);
  height = utility::readShort(stream);

  //unknown2 = utility::readLong(stream); // always 0
  //unknown3 = utility::readLong(stream); // always 0
  //unknown4 = utility::readLong(stream); // always 0
  //unknown5 = utility::readLong(stream); // always 0
  //unknown6 = utility::readLong(stream); // always 0
  //unknown7 = utility::readLong(stream); // always 0

  stream->ignore(6 * 4);

  if (nMipmap > 1) {
    mipmap_size.resize(nMipmap);
    for (int i = 0; i < nMipmap; i++) {
      mipmap_size[i] = utility::readLong(stream);
    }
  }

          // skip padding for 16byte alignment

  utility::evenReadStream(stream, 16);
  //stream.ignore((16 - stream.tellg() % 16) % 16);

          // EXT
  std::string ext_label;
  ext_label.resize(4);
  stream->read(ext_label.data(), 4);
  if(std::string{'e', 'X', 't', '\0'} != ext_label) {
    return false;
  }

  ext.param1 = utility::readLong(stream);  // always 32
  ext.param2 = utility::readLong(stream);  // always 16
  ext.param3 = utility::readLong(stream); // always 0

          // GIDX
  std::string gidx_label;
  gidx_label.resize(4);
  stream->read(gidx_label.data(), 4);
  if("GIDX" != gidx_label) {
    return false;
  }

  gidx.unknown11 = utility::readLong(stream); // always 16
  gidx.GIDX = utility::readLong(stream);
  gidx.unknown12 = utility::readLong(stream); // always 0

  raw_texture.resize(image_data_size);
  stream->read(raw_texture.data(), image_data_size);
  return true;
}

void TextureData::write(std::basic_ostream<char> *stream) {
  auto const header_size = headerSize();

  int32_t const image_data_size = raw_texture.size();
  int32_t const texture_data_size = header_size + image_data_size;

  utility::writeLong(stream, texture_data_size);
  utility::writeLong(stream, unknown0);
  utility::writeLong(stream, image_data_size);
  utility::writeShort(stream, header_size);
  utility::writeShort(stream, unknown1);
  utility::writeShort(stream, nMipmap);
  utility::writeShort(stream, pixel_type);
  utility::writeShort(stream, width);
  utility::writeShort(stream, height);

  utility::padStream(stream, 0, 6 * 4);

  if (mipmap_size.size() > 1) {
    for (int i = 0; i < mipmap_size.size(); i++) {
      utility::writeLong(stream, mipmap_size[i]);
    }
    utility::evenWriteStream(stream);
    //pFile->WriteNullArray((16 - mipmap_size.size() % 16) % 16);
  }

  stream->write("eXt\0", 4);
  utility::writeLong(stream, ext.param1);
  utility::writeLong(stream, ext.param2);
  utility::writeLong(stream, ext.param3);

  stream->write("GIDX", 4);
  utility::writeLong(stream, gidx.unknown11);
  utility::writeLong(stream, gidx.GIDX);
  utility::writeLong(stream, gidx.unknown12);

  stream->write(raw_texture.data(), raw_texture.size());
}

int16_t TextureData::headerSize() const {
  return 48 + (mipmap_size.size() > 1 ? ((mipmap_size.size() - 1) / 16 + 1) * 16 : 0) + 16 + 16;
}

int32_t TextureData::calculateSize() const {
  return headerSize() + raw_texture.size();
}

boost::json::value TextureData::toJson() const {
  boost::json::object root;
  root["pixel_type"] = pixel_type;
  root["width"] = width;
  root["height"] = height;
  root["size"] = raw_texture.size();
  boost::json::object mipmap;
  mipmap["size"] = nMipmap;
  boost::json::array m_array(mipmap_size.begin(), mipmap_size.end());
  mipmap["array"] = m_array;
  root["mipmap"] = mipmap;
  root["eXt"] = { { "param1", ext.param1 }, { "param2", ext.param2 }, { "param3", ext.param3 } };
  root["GIDX"] = { { "param1", gidx.unknown11 }, { "param2", gidx.GIDX }, { "param3", gidx.unknown12 } };
  return boost::json::value(root);
}

Result TextureData::fromJson(boost::json::value const& value) {
  auto root = value.as_object();
  try{
    pixel_type = root["pixel_type"].as_int64();
    width = root["width"].as_int64();
    height = root["height"].as_int64();
    auto mipmap = root["mipmap"].as_object();
    nMipmap = mipmap["size"].as_int64();
    if(nMipmap > 1) {
      auto const mipmaps = mipmap["array"].as_array() | boost::adaptors::transformed([](boost::json::value& value){
                       return value.as_int64();
                     });
      mipmap_size.assign(mipmaps.begin(), mipmaps.end());
    }

    auto ext_value = root["eXt"].as_object();
    ext.param1 = ext_value["param1"].as_int64();
    ext.param2 = ext_value["param2"].as_int64();
    ext.param3 = ext_value["param3"].as_int64();
    auto gidx_value = root["GIDX"].as_object();
    gidx.unknown11 = gidx_value["param1"].as_int64();
    gidx.GIDX = gidx_value["param2"].as_int64();
    gidx.unknown12 = gidx_value["param3"].as_int64();

  } catch (std::invalid_argument const& e) {
    return {false, "Failed to read the meta.json file. Wrong value format: " + std::string(e.what())};
  }
  return {true, ""};
}

Result TextureData::exportDDS(std::filesystem::path const& extract_dir_path) const
{
  DDS_HEADER header{0};

  header.dwMagic = ' SDD';
  header.dwSize = 124;
  header.dwFlags = 0x000A1007;
  header.dwHeight = height;
  header.dwWidth = width;
  header.dwMipMapCount = nMipmap;

  header.ddpfPixelFormat.dwSize = 32;
  header.ddpfPixelFormat.dwFlags = 0x000004;

  switch (pixel_type) {
    case 0:
      header.ddpfPixelFormat.dwFourCC = '1TXD';
      break;
    case 1:
      header.ddpfPixelFormat.dwFourCC = '3TXD';
      break;
    case 2:
      header.ddpfPixelFormat.dwFourCC = '5TXD';
      break;
    case 19:
      // RGB32
      [[fallthrough]];
    case 20:
      // ARGB32
      header.dwFlags = 0x0002100F;
      header.ddpfPixelFormat.dwFlags = 0x00000041;
      header.ddpfPixelFormat.dwFourCC = 0;

      header.ddpfPixelFormat.dwRGBBitCount = 32;
      header.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
      header.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
      header.ddpfPixelFormat.dwBBitMask = 0x000000FF;
      header.ddpfPixelFormat.dwRGBAlphaBitMask = 0xFF000000;
  }

  header.ddsCaps.dwCaps1 = 0x00401008;

  std::vector<char> dds_data;

  dds_data.resize(sizeof(header) + raw_texture.size());
  memcpy(&dds_data[0], &header, sizeof(header));

  auto texture_tmp = raw_texture;
  int pixel_type = pixel_type;
  if (pixel_type == 0 || pixel_type == 1 || pixel_type == 2) {
    if (0 != texture_tmp.size() % 2) {
      return {false, "Wrong texture block size. (Texture size should be power of 2)"};
    }
    changeEndian2(texture_tmp);
  } else {
    if (0 != texture_tmp.size() % 4) {
      return {false, "Wrong texture block size. (Texture size should be power of 4)"};
    }
    changeEndian4(texture_tmp);
  }

  std::ranges::copy(texture_tmp, dds_data.begin() + sizeof(header));

  std::ostringstream fname_steam;
  auto const filename = extract_dir_path.filename();
  fname_steam << filename.string() << "_" << gidx.GIDX << ".dds";
  auto const final_filename = fname_steam.str();
  auto const final_path = extract_dir_path.parent_path() / final_filename;

  std::ofstream stream(final_path, std::ios_base::binary);
  if (!stream.is_open()) {
    return {false, "Failed to open the save file."};
  }
  stream.write(dds_data.data(), dds_data.size());
  return {true, final_path.string()};
}

//Replaces the contents of the texture with the contents of the DDS file
Result TextureData::importDDS(const std::filesystem::path& filepath)
{
  std::ifstream stream(filepath, std::ios_base::binary);
  if (!stream.is_open()) {
    return {false, "Failed to open the file."};
  }
  DDS_HEADER header;
  stream.read(reinterpret_cast<char*>(&header), sizeof(header));
  if (header.dwMagic != ' SDD') {
    return {false, "Not a DDS file."};
  }
  width = header.dwWidth;
  height = header.dwHeight;
  nMipmap = header.dwMipMapCount;
  mipmap_size.resize(nMipmap);
  auto const texture_size = std::filesystem::file_size(filepath) - sizeof(DDS_HEADER);
  raw_texture.resize(texture_size);
  stream.read(raw_texture.data(), texture_size);
  switch (header.ddpfPixelFormat.dwFourCC)
  {
    case '1TXD':
      pixel_type = 0;
      break;
    case '3TXD':
      pixel_type = 1;
      break;
    case '5TXD':
      pixel_type = 2;
      break;
    case 0:
      pixel_type = 20;
      break;
    default:
      return {false, "Not a DDS file."};
  }
  if (pixel_type == 0 || pixel_type == 1 || pixel_type == 2) {
    if (0 != raw_texture.size() % 2) {
      return {false, "Wrong texture block size. (Texture size should be power of 2)"};
    }
    changeEndian2(raw_texture);
  } else {
    if (0 != raw_texture.size() % 4) {
      return {false, "Wrong texture block size. (Texture size should be power of 4)"};
    }
    changeEndian4(raw_texture);
  }
  return {true, filepath.string()};
}

Result NUT::openFromStream(std::basic_istream<char> *stream) {
  if (utility::readLong(stream) != 'NTXR') {
    return {false, "Wrong signature in the file. Probably not a NUT file."};
  }

  unknown0 = utility::readShort(stream); // always 256

  int texture_count;
  texture_count = utility::readShort(stream);

  unknown1 = utility::readShort(stream); // always 0
  unknown2 = utility::readShort(stream); // always 0
  unknown3 = utility::readShort(stream); // always 0
  unknown4 = utility::readShort(stream); // always 0

  texture_data.resize(texture_count);
  for (int i = 0; i < texture_count; i++) {
    texture_data[i].load(stream);
  }

  return {true, "Successfully loaded NUT file."};
}

Result NUT::saveToStream(std::basic_ostream<char> *stream) {
  if(texture_data.size() == 0) {
    return {false, "No texture data to save."};
  }

  stream->write("NTXR", 4);

  utility::writeShort(stream, unknown0);
  utility::writeShort(stream, texture_data.size());
  utility::writeShort(stream, unknown1);
  utility::writeShort(stream, unknown2);
  utility::writeShort(stream, unknown3);
  utility::writeShort(stream, unknown4);

  for (int i = 0; i < texture_data.size(); i++) {
    texture_data[i].write(stream);
  }
  return {true, "Successfully saved NUT file."};
}

Result NUT::extract(std::filesystem::path const& savepath) const {
  bjson::object nut_data;
  nut_data["texture_count"] = (int)texture_data.size();
  nut_data["unknown0"] = unknown0;
  nut_data["unknown1"] = unknown1;
  nut_data["unknown2"] = unknown2;
  nut_data["unknown3"] = unknown3;
  nut_data["unknown4"] = unknown4;
  bjson::object texture_value;
  for (auto const& texture : texture_data) {
    texture_value[std::to_string(texture.gidx.GIDX)] = texture.toJson();
  }
  nut_data["texture_data"] = texture_value;

#ifdef NUT_WRITE_META
  std::ostringstream fname_steam;
  fname_steam << savepath.filename().string() << "_meta.json";
  auto const final_filename = fname_steam.str();
  auto const final_path = savepath.parent_path() / final_filename;
  std::ofstream stream(final_path, std::ios_base::binary);
  if (!stream.is_open()) {
    return {false, "Failed to write the meta.json file."};
  }
  stream << bjson::value(nut_data);
#endif

  std::stringstream result_str;
  result_str << "Exporting " << texture_data.size() << " textures to DDS format..." << std::endl << "Exported files:" << std::endl;
  for (auto const& texture : texture_data) {
    if(auto const res = texture.exportDDS(savepath); res.first){
      result_str << res.second;
    }else{
      return res;
    }
  }
  return {true, result_str.str()};
}

Result NUT::inject(const std::filesystem::path& dirpath) {
  size_t texture_count = 0;
  for (auto& texture : texture_data) {
    std::ostringstream fname_steam;
    auto const last_dir = dirpath.filename();
    fname_steam << last_dir.string() << "_" << texture.gidx.GIDX << ".dds";
    auto const filename = fname_steam.str();
    auto const endpath = dirpath.parent_path() / filename;
    if (!std::filesystem::exists(endpath)) {
      continue;
    }
    if(auto const res = texture.importDDS(endpath); !res.first){
      return {false, filename + " failed to load: " + res.second};
    }
    ++texture_count;
  }
  if(0 == texture_count) {
    return {false, "Failed to import textures: nothing to import."};
  }
  std::stringstream result_str;
  result_str << "Imported " << texture_count << " textures.";
  return {true, result_str.str()};
}

Manageable::Fileapi NUT::api() const{
  static auto const api = Fileapi{.base_extension = "nut",
                          .type = ExtractType::dir,
                          .extraction_title = "Extract DDS...",
                          .injection_title = "Import DDS..."};
  return api;
}

Result NUT::loadDDS(std::filesystem::path const& dirpath)
{
  reset();
  //read metadata
  std::ifstream stream(dirpath / "meta.json", std::ios_base::binary);
  if (!stream.is_open()) {
    return {false, "Failed to read the meta.json file."};
  }
  bjson::value nut_value;
  stream >> nut_value;
  auto& nut_data = nut_value.as_object();
  int64_t texture_count;
  try {
    unknown0 = nut_data["unknown0"].as_int64();
    unknown1 = nut_data["unknown1"].as_int64();
    unknown2 = nut_data["unknown2"].as_int64();
    unknown3 = nut_data["unknown3"].as_int64();
    unknown4 = nut_data["unknown4"].as_int64();
    texture_count = nut_data["texture_count"].as_int64();
  } catch (std::invalid_argument const& e) {
    return {false, "Failed to read the meta.json file. Wrong value format: " + std::string(e.what())};
  }
  //collect DDS
  std::vector<std::filesystem::path> dds_paths;
  for(auto& path: std::filesystem::directory_iterator(dirpath)) {
    if (path.path().extension() == ".dds") {
      dds_paths.push_back(path.path());
    }
  }
  if(dds_paths.size() == 0) {
    return {false, "No DDS files found."};
  }
  if(dds_paths.size() != texture_count) {
    return {false, "The number of DDS files does not match the number in the meta.json file. Expected " + std::to_string(texture_count) + " but found " + std::to_string(dds_paths.size())};
  }
  std::sort(dds_paths.begin(), dds_paths.end());
  texture_data.reserve(dds_paths.size());
  for(auto const& path: dds_paths) {
    auto &texture = texture_data.emplace_back();
    if(auto const res = texture.fromJson(nut_data["texture_data"].as_object()[path.stem().string()]); !res.first) {
      return {false, path.string() + " failed to load texture metadata: " + res.second};
    }
    if(auto const res = texture.importDDS(path); !res.first){
      return {false, path.string() + " failed to load: " + res.second};
    }
  }
  return {true, "Successfully loaded DDS files."};
}

void NUT::reset() {
  texture_data.clear();
}

size_t NUT::size() const
{
  ByteCounter counter { 16 }; // header size
  for(auto const& section: texture_data) {
      counter.addSize(section.calculateSize());
      counter.pad(0x10);
  }
  return counter.offset;
}

} // namespace file
} // namespace imas
