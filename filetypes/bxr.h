#pragma once

#include "filetypes/manageable.h"
#include <filesystem>
#include <fstream>
#include <numeric>
#include <string>
#include <vector>

#include <QJsonArray>

//#define BXR_DEBUG_LINKS

//*********.bxr
//1. Label("BXR0")
//2. Size data(tag main, tag sub, main, sub, string = 5*32)
//2. Entry data
//2.1 Tag main
//2.2 Tag sub
//2.3 Main
//2.4 Sub
//3.String chunk
//3.1 8 bit-wide strings(ANSI)
//3.2 16 bit-wide strings(Japanese characters)

namespace imas {
namespace file {

class BXR : public Manageable
{
public:
    BXR();
    // Manageable interface
    virtual void loadFromData(const std::vector<char>& data) override;
    virtual void loadFromFile(std::filesystem::path const& filename) override;
    virtual void saveToData(std::vector<char>& data) override;
    virtual void saveToFile(std::filesystem::path const& filename) override;
    virtual std::pair<bool, std::string> extract(const std::filesystem::path& savepath) override{
      return writeXML(savepath);
    }
    virtual std::pair<bool, std::string> inject(const std::filesystem::path& openpath) override{
      return readXML(openpath);
    }
    void reset();

    //QJsonArray getJson();
    //std::pair<bool, std::string> setJson(const QJsonValue &json);

    std::pair<bool, std::string> readXML(std::filesystem::path const& filepath);
    std::pair<bool, std::string> writeXML(std::filesystem::path const& filepath);

  private:
    struct Offsetable {
      //raw data
      int32_t offset = -1;
      //convenience data
      std::string symbol;
    };

#ifdef BXR_DEBUG_LINKS
    template<class T>
    struct OffsetData : Offsetable
    {
        std::vector<T *> children;
    };
#endif

    struct MainScriptEntry;

    struct OffsetTaggable : Offsetable {
        //raw data
        int data_tag = -1;               //index of a tag in a related tag list
        //convenience data
        std::string_view tag_view;
    };

    struct SubScriptEntry : OffsetTaggable {
        int next;
#ifdef BXR_DEBUG_LINKS
        std::optional<MainScriptEntry*> parent;
#endif
    };

    struct MainScriptEntry : OffsetTaggable
    {
        //raw data
        int before = -1;             //Previous element, seems to be lineary incresiable
        int index_sub_item = -1;     //Index of the first subitem element. Amount of child elements decided by the subitem's 'next' field
        int next;               //Next element, seems to be lineary as well
        int next_ticks;         //Denotes the element starting the next tag. Aka, all elements between this and 'next_ticks' ones are children of this.
        int offset_unicode = -1;     //Points to the unicode string
        //convenience data
        int power = 0;              //denoutes the level in the hierarchy. Derived from the 'next_ticks'
        std::u16string unicode;
#ifdef BXR_DEBUG_LINKS
        std::vector<MainScriptEntry *> children;
#endif
        std::vector<SubScriptEntry *> subscript_children;
    };

#ifdef BXR_DEBUG_LINKS
    std::vector<OffsetData<MainScriptEntry>> tag_main;
    std::vector<OffsetData<SubScriptEntry>> tag_sub;
#else
    std::vector<Offsetable> m_main_tags;
    std::vector<Offsetable> m_sub_tags;
#endif

    std::vector<MainScriptEntry> m_main_items;
    std::vector<SubScriptEntry> m_sub_items;
    std::string m_property_name = "symbol";

    template<class S> std::pair<bool, std::string> openFromStream(S &stream);
    template<class S> std::pair<bool, std::string> saveToStream(S &stream);
    //size calculation
    uint32_t getUnicodeSize() const;
    uint32_t calculateSize() const;
    uint32_t calculateStringSize() const;
    //rebuilding
    void setUnicodeData(std::vector<char>& output);
    template<class T>
    void insertIntoQueue(std::vector<T>& origin, std::vector<imas::file::BXR::Offsetable*>& output);
};

} // namespace file
} // namespace imas
