#pragma once

#include "filetypes/manageable.h"

namespace imas {
namespace file {

class LBL : public Manageable {
public:
  LBL();

  // Manageable interface
  public:
  virtual Fileapi api() const override{
    static auto const api = Fileapi{.base_extension = "lbl",
                            .final_extension = "csv",
                            .type = ExtractType::file,
                            .signature = "Comma separated values (*.CSV)",
                            .extraction_title = "Extract labels...",
                            .injection_title = "Import labels..."};
    return api;
  }
  virtual Result extract(const std::filesystem::__cxx11::path& savepath) const override{
    return {true, ""};
  }
  virtual Result inject(const std::filesystem::__cxx11::path& openpath) override{
    return {true, ""};
  }

  protected:
  virtual Result openFromStream(std::basic_istream<char>* stream) override{
    std::string label;
    label.resize(4);
    stream->read(label.data(), 4);


    return {true, ""};
  }
  virtual Result saveToStream(std::basic_ostream<char>* stream) override{
    return {true, ""};
  }
  virtual size_t size() const override{
    return 0;
  }
};

} // namespace file
} // namespace imas
