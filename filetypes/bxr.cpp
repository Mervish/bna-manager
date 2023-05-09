#include "bxr.h"

#include "utility/streamtools.h"

#include <filesystem>

#include <QDebug>
#include <QFile>
#include <QXmlStreamWriter>

#include <boost/range/adaptors.hpp>

namespace adaptor = boost::adaptors;

namespace {
constexpr auto bxr_label = "BXR0";

template<class T>
uint32_t getStringSize(std::vector<T> const& evaluated) {
    return std::accumulate(evaluated.begin(), evaluated.end(), 0, [](const auto &a, const auto &b){
        return a + (b.symbol.size() + 1);
    });
}

uint32_t getStringSize(std::vector<imas::file::BXR::MainScriptEntry> const& evaluated) {
    return std::accumulate(evaluated.begin(), evaluated.end(), 0, [](const auto &a, const imas::file::BXR::MainScriptEntry &b){
        auto const symbol_size  = -1 == b.offset_symbol ? 0 :(b.symbol.size() + 1);
        auto const unicode_size = -1 == b.offset_unicode ? 0 :(b.unicode.size() + 1) * 2;
        return a + symbol_size + unicode_size;
    });
}

template<class T>
void setStringData(std::vector<T>& manipulated, std::vector<char>& output) {
    for(auto& entry: manipulated) {
        entry->offset_symbol = output.size();
        std::ranges::copy(entry->symbol, std::back_insert_iterator(output));
        output.push_back('\0');
    }
}

template<class T>
void putToVector(std::vector<T>& origin, std::vector<imas::file::BXR::Offsetable*>& output) {
    auto transformed_range = origin | adaptor::filtered([](T& value){
                                 return -1 != value.offset_symbol;
                             }) | adaptor::transformed([](T& value){
                                 return &value;
                             });
    output.insert(output.end(), transformed_range.begin(), transformed_range.end());
}

void setUnicodeData(std::vector<imas::file::BXR::MainScriptEntry>& manipulated, std::vector<char>& output) {
    for(auto& entry: manipulated) {
        if( -1 != entry.offset_unicode) {
            entry.offset_unicode = output.size();
            auto reversed = entry.unicode;
            for(auto& sym: reversed) {
                _byteswap_ushort(sym);
            }
            std::ranges::copy(reversed, std::back_insert_iterator(output));
            output.push_back('\0');
            output.push_back('\0');
        }
    }
}


struct SectionSizes {
    unsigned int tag_main_script;
    unsigned int tag_sub_script;
    unsigned int main_script;
    unsigned int sub_script;
    unsigned int symbol_offset;
};
}

namespace imas{
namespace file{

bool BXR::load(std::string filename)
{
    std::ifstream stream(filename, std::ios_base::binary);

    std::string label;
    label.resize(4);
    stream.read(label.data(), 4);

    if(bxr_label != label) {
        return false;
    }

    SectionSizes sizes;

    //First - we read offset array sizes
    sizes.tag_main_script = imas::tools::readLong(stream);
    sizes.tag_sub_script  = imas::tools::readLong(stream);
    sizes.main_script     = imas::tools::readLong(stream);
    sizes.sub_script      = imas::tools::readLong(stream);
    sizes.symbol_offset   = imas::tools::readLong(stream);

    // BLOCK1
    // Then we read main script tag offset
    tag_main.resize( sizes.tag_main_script );
    for(auto& entry: tag_main) {
        entry.offset_symbol = imas::tools::readLong(stream);
    }

    // BLOCK2
    // And a subscript tag offset
    tag_sub.resize( sizes.tag_sub_script );
    for(auto& entry: tag_sub) {
        entry.offset_symbol = imas::tools::readLong(stream);
    }

    // BLOCK3
    // We fill the mainscript entries with the integer data
    main_script.resize( sizes.main_script );
    for(auto& entry: main_script)
    {
        entry.before           = imas::tools::readLong(stream);
        entry.next             = imas::tools::readLong(stream);
        entry.data_tag         = imas::tools::readLong(stream);
        entry.offset_symbol    = imas::tools::readLong(stream);
        entry.index_sub_script = imas::tools::readLong(stream);
        entry.offset_unicode   = imas::tools::readLong(stream);
        entry.next_ticks   = imas::tools::readLong(stream);
    }

    // BLOCK4
    // Ditto with subscript
    sub_script.resize( sizes.sub_script );
    for(auto& entry: sub_script)
    {
        entry.next         = imas::tools::readLong(stream);
        entry.data_tag      = imas::tools::readLong(stream);
        entry.offset_symbol = imas::tools::readLong(stream);
    }

    // BLOCK5
    // Then we read some sort of symbolic data
    std::vector<char> symbol(sizes.symbol_offset);
    stream.read(symbol.data(), symbol.size());

    //
    // 解釈
    //

    // BLOCK1
    // Then we fill symbol data for the tag and script sections
    for(auto& entry : tag_main) {
        entry.symbol.assign(&symbol.data()[ entry.offset_symbol ]);
    }

    // BLOCK2
    for(auto& entry : tag_sub) {
        entry.symbol.assign(&symbol.data()[ entry.offset_symbol ]);
    }

    // BLOCK3    
    //Setting substrings
    for(auto iter = main_script.begin(); iter != main_script.end();++iter) {
        if( -1 != iter->index_sub_script) {
            auto sub_iter = sub_script.begin() + iter->index_sub_script;
            while(sub_iter != sub_script.end()) {
                iter->subscript_children.push_back(&*sub_iter);
                sub_iter->parent = &*iter;
                if(-1 == sub_iter->next) {
                    break;
                }
                ++sub_iter;
            }
        }
    }

    //So, basically, tags are recorded lineary, like this:
    //<root>
    //<title>
    //<data>
    //'next_ticks' defines the end of the current group, so if we use it as reference, we should get something like this
    //<root>
    //  <title>
    //      <data>
    //Thus recreating the xml's hierarchy

    //1-st pass: we put weights based on the
    std::vector<std::pair<MainScriptEntry*,int>> base_entry_list;
    for(auto& entry: main_script) {
        base_entry_list.emplace_back(&entry, 0);
    }

    //So, 'next_ticks' defines the end of the current group, so we raise the weights for all affected entries.
    //If we go over all the entries and shift the weight, then we can recreate xml-structure.
    for(auto iter = base_entry_list.begin(); iter != base_entry_list.end(); ++iter) {
        if (-1 != iter->first->next_ticks) {
            auto end_brace = base_entry_list.begin() + (iter->first->next_ticks);
            if(end_brace > base_entry_list.end()) {
                end_brace = base_entry_list.end();
            }
            std::for_each(iter + 1, end_brace, [](auto& pair){
                ++pair.second;
            });
        }
    }

    /*root = &main_script.front();

    for(auto iter = base_entry_list.begin(); iter != base_entry_list.end(); ++iter) {
        auto const base_level = iter->second;
        auto sub_iter = iter + 1;
        while(sub_iter != base_entry_list.end()) {
            auto const level = sub_iter->second;
            if(level <= base_level) {
                break;
            } else {
                iter->first->children.push_back(sub_iter->first);
            }
        }
    }*/

    /*for (size_t ind = 0; ind < base_entry_list.size(); ++ind) {
        auto& [entry, power] = base_entry_list[ind];
        if (-1 != entry->next_ticks) {
        }
    }*/

    for(auto& entry : main_script)
    {
        auto& parent = tag_main[ entry.data_tag ];
        parent.children.push_back(&entry);
        entry.main_symbol = parent.symbol;
        if( -1 != entry.offset_symbol ) {
            entry.symbol.assign(&symbol.data()[ entry.offset_symbol ]);
        }

        // 歌詞など
        if( -1 != entry.offset_unicode )
        {
            entry.unicode.assign( (char16_t*) & symbol[ entry.offset_unicode ]);
            //Curiously, symbol data has mixed 8 and 16 bit-wide strings
            //So we need to convert the endianess on the fly
            for(auto& character: entry.unicode) {
                character = _byteswap_ushort(character);
            }
        }
    }

    // BLOCK4
    for(auto& entry: sub_script)
    {
        auto& parent = tag_sub[ entry.data_tag ];
        parent.children.push_back(&entry);
        entry.sub_symbol = parent.symbol;
        entry.symbol = std::string(&symbol.data()[ entry.offset_symbol ]);
    }

    qInfo() << "filesize = " << std::filesystem::file_size(filename);
    qInfo() << "calculated size = " << calculateSize();

    //Test output
    for(auto const& [entry, power]: base_entry_list) {
        QString tag = std::string(entry->main_symbol).c_str();
        auto const symbol = QString::fromStdString(entry->symbol);
        if(!symbol.isEmpty()) {
            tag.append(" symbol=" + symbol);
        }
        auto const unicode = QString::fromStdU16String(entry->unicode);
        if(!unicode.isEmpty()) {
            tag.append(" unicode=" + unicode);
        }
        qInfo() << QString(power, ' ') + QString("<%1>").arg(tag);
        if(!entry->subscript_children.empty())
        {
            for(auto child: entry->subscript_children){
                qInfo() << QString(power + 1, ' ') + QString("%1=%2").arg(std::string(child->sub_symbol).c_str()).arg(child->symbol.c_str());
            }
        }
    }



    return true;
}

void BXR::writeXML(std::string const& filename) {
    //Test XML parsing
    QFile file(QString::fromStdString(filename));
    if(!file.open(QIODevice::WriteOnly)){
        return;
    }
    QXmlStreamWriter stream(&file);
    stream.setAutoFormatting(true);
    std::vector<std::pair<MainScriptEntry*,int>> base_entry_list;
    for(auto& entry: main_script) {
        base_entry_list.emplace_back(&entry, 0);
    }

    for(auto iter = base_entry_list.begin(); iter != base_entry_list.end(); ++iter) {
        if (-1 != iter->first->next_ticks) {
            auto end_brace = base_entry_list.begin() + (iter->first->next_ticks);
            if(end_brace > base_entry_list.end()) {
                end_brace = base_entry_list.end();
            }
            std::for_each(iter + 1, end_brace, [](auto& pair){
                ++pair.second;
            });
        }
    }

    auto iter = base_entry_list.begin();
    while (iter != base_entry_list.end()) {
        auto const &[entry, power] = *iter;
        auto const symbol = QString::fromStdString(entry->symbol);
        auto const unicode = QString::fromStdU16String(entry->unicode);
        stream.writeStartElement(QString::fromStdString(std::string(entry->main_symbol)));
        if (!symbol.isEmpty()) {
            stream.writeAttribute("symbol", symbol);
        }
        if (!unicode.isEmpty()) {
            stream.writeAttribute("unicode", unicode);
        }
        if (!entry->subscript_children.empty()) {
            for (auto child : entry->subscript_children) {
                stream.writeTextElement(child->sub_symbol, child->symbol);
            }
            //stream.writeEndElement();
        }
        auto const next = iter + 1;
        if (next != base_entry_list.end()) {
            auto power_diff = iter->second - next->second;
            if (power_diff >= 0) {
                stream.writeEndElement();
                for (int d = 0; d < power_diff; ++d) {
                    stream.writeEndElement();
                }
            }
        }
        ++iter;
    }
    stream.writeEndDocument();
}

uint32_t BXR::calculateStringSize()
{
    return getStringSize(tag_main) + getStringSize(tag_sub) + getStringSize(main_script)
           + getStringSize(sub_script) + 1;
}

uint32_t BXR::calculateSize() {
    uint32_t str_tag_main = getStringSize(tag_main);
    uint32_t str_tag_sub = getStringSize(tag_sub);
    uint32_t str_main = getStringSize(main_script);
    uint32_t str_sub = getStringSize(sub_script);

    uint32_t const string_size = str_tag_main + str_tag_sub + str_main + str_sub + 1; //The string chunk has one extra terminating zero
    uint32_t label = 4;                             //label
    uint32_t offsets = (5 * 4);                        //offsets
    uint32_t tag_m = (4 * tag_main.size()); //Tag data (1 param = 8 bit)
    uint32_t tag_s = (4 * tag_sub.size());  //
    uint32_t main = (4 * 7 * main_script.size());
    uint32_t sub = (4 * 3 * sub_script.size());
    uint32_t strings = string_size;
    return label + offsets + tag_m + tag_s + main + sub + strings;
}

void BXR::save(const std::string& filename) {
    std::ofstream stream(filename, std::ios_base::binary);

    //1. Let's start building the string chunk
    std::vector<Offsetable*> offset_map;

    putToVector(tag_main, offset_map);
    putToVector(tag_sub, offset_map);
    putToVector(main_script, offset_map);
    putToVector(sub_script, offset_map);

    std::ranges::sort(offset_map, [](Offsetable const* left, Offsetable const* right){
        return left->offset_symbol < right->offset_symbol;
    });

    std::vector<char> string_library;
    string_library.reserve(calculateStringSize());
    setStringData(offset_map, string_library);
    setUnicodeData(main_script, string_library);
    string_library.push_back('\0'); //Additional terminating symbol

    //Write label
    std::string label{bxr_label};
    stream.write(label.data(), label.size());

    //Write base offsets
    imas::tools::writeLong(stream, tag_main.size());
    imas::tools::writeLong(stream, tag_sub.size());
    imas::tools::writeLong(stream, main_script.size());
    imas::tools::writeLong(stream, sub_script.size());
    imas::tools::writeLong(stream, string_library.size());

    //Write sections data
    for(auto& entry: tag_main) {
        imas::tools::writeLong(stream, entry.offset_symbol);
    }

    // BLOCK2
    // And a subscript tag offset
    for(auto& entry: tag_sub) {
        imas::tools::writeLong(stream, entry.offset_symbol);
    }

    // BLOCK3
    // We fill the mainscript entries with the integer data
    for(auto& entry: main_script)
    {
        imas::tools::writeLong(stream, entry.before          );
        imas::tools::writeLong(stream, entry.next            );
        imas::tools::writeLong(stream, entry.data_tag        );
        imas::tools::writeLong(stream, entry.offset_symbol   );
        imas::tools::writeLong(stream, entry.index_sub_script);
        imas::tools::writeLong(stream, entry.offset_unicode  );
        imas::tools::writeLong(stream, entry.next_ticks      );
    }

    // BLOCK4
    // Ditto with subscript
    for(auto& entry: sub_script)
    {
        imas::tools::writeLong(stream, entry.next         );
        imas::tools::writeLong(stream, entry.data_tag     );
        imas::tools::writeLong(stream, entry.offset_symbol);
    }

    //. Write string chunk
    stream.write(string_library.data(), string_library.size());
}

QJsonArray BXR::getJson()
{
    QJsonArray BXR_json;
    for (auto const &entry : unicode_containing_entries) {
        BXR_json.append(QString::fromStdU16String(entry.get().unicode));
    }
    return BXR_json;
}

std::pair<bool, std::string> BXR::setJson(const QJsonValue& json)
{
    auto const array = json.toArray();
    if (array.size() != unicode_containing_entries.size()) {
        return {false, "BXR string injection error: Count mismatch. Expected " + std::to_string(unicode_containing_entries.size()) + ", got " + std::to_string(array.size()) + ". Are you loading the correct file?"};
    }

    for (size_t ind = 0; ind < unicode_containing_entries.size(); ++ind) {
        unicode_containing_entries[ind].get().unicode = array.at(ind).toString().toStdU16String();
    }
    return {true, ""};
}

}
}
