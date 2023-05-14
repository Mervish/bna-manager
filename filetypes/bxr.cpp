#include "bxr.h"

#include "utility/streamtools.h"

#include <QDebug>
#include <QFile>
#include <QXmlStreamWriter>

#include <boost/range/adaptors.hpp>

namespace adaptor = boost::adaptors;

namespace {
constexpr auto bxr_label = "BXR0";
constexpr auto unicode_literal = "unicode";
constexpr auto type_hack_literal = "type_hack";

template<class T>
uint32_t getStringSize(std::vector<T> const& evaluated) {
    return std::accumulate(evaluated.begin(), evaluated.end(), 0, [](const auto &a, const auto &b){
        return a + (-1 == b.offset ? 0 :b.symbol.size() + 1);
    });
}

template<class T>
void setStringData(std::vector<T>& origin, std::vector<char>& output) {
    for(auto& entry: origin) {
        entry->offset = output.size();
        std::ranges::copy(entry->symbol, std::back_insert_iterator(output));
        output.push_back('\0');
    }
}

struct SectionSizes {
    unsigned int tag_main_script;
    unsigned int tag_sub_script;
    unsigned int main_script;
    unsigned int sub_script;
    unsigned int symbol_offset;
};

enum class CandidateType
{
    undef,
    main,
    sub
};

struct Candidate {
    std::string tag;
    std::string value;
    std::u16string unicode;
    int power;
    CandidateType type;
};

template <CandidateType F>
bool filterCandidateType(Candidate const& entry) {
    return F == entry.type;
}
}

namespace imas{
namespace file{

std::pair<bool, std::string> BXR::load(std::filesystem::path const &filepath)
{
    std::ifstream stream(filepath, std::ios_base::binary);

    std::string label;
    label.resize(4);
    stream.read(label.data(), 4);

    if(bxr_label != label) {
        return {false, "Wrong filetype? File label mismatch."};
    }
    reset();

    SectionSizes sizes;

    //First - we read offset array sizes
    sizes.tag_main_script = imas::tools::readLong(stream);
    sizes.tag_sub_script  = imas::tools::readLong(stream);
    sizes.main_script     = imas::tools::readLong(stream);
    sizes.sub_script      = imas::tools::readLong(stream);
    sizes.symbol_offset   = imas::tools::readLong(stream);

    // BLOCK1
    // Then we read main script tag offset
    m_main_tags.resize( sizes.tag_main_script );
    for(auto& entry: m_main_tags) {
        entry.offset = imas::tools::readLong(stream);
    }

    // BLOCK2
    // And a subscript tag offset
    m_sub_tags.resize( sizes.tag_sub_script );
    for(auto& entry: m_sub_tags) {
        entry.offset = imas::tools::readLong(stream);
    }

    // BLOCK3
    // We fill the mainscript entries with the integer data
    m_main_items.resize( sizes.main_script );
    for(auto& entry: m_main_items)
    {
        entry.before           = imas::tools::readLong(stream);
        entry.next             = imas::tools::readLong(stream);
        entry.data_tag         = imas::tools::readLong(stream);
        entry.offset         = imas::tools::readLong(stream);
        entry.index_sub_item = imas::tools::readLong(stream);
        entry.offset_unicode   = imas::tools::readLong(stream);
        entry.next_ticks   = imas::tools::readLong(stream);
    }

    // BLOCK4
    // Ditto with subscript
    m_sub_items.resize( sizes.sub_script );
    for(auto& entry: m_sub_items)
    {
        entry.next         = imas::tools::readLong(stream);
        entry.data_tag      = imas::tools::readLong(stream);
        entry.offset = imas::tools::readLong(stream);
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
    for(auto& entry : m_main_tags) {
        entry.symbol.assign(&symbol.data()[ entry.offset ]);
    }

    // BLOCK2
    for(auto& entry : m_sub_tags) {
        entry.symbol.assign(&symbol.data()[ entry.offset ]);
    }

    // BLOCK3    
    //Setting substrings
    for(auto iter = m_main_items.begin(); iter != m_main_items.end();++iter) {
        if( -1 != iter->index_sub_item) {
          auto sub_iter = m_sub_items.begin() + iter->index_sub_item;
            while(sub_iter != m_sub_items.end()) {
                iter->subscript_children.push_back(&*sub_iter);
#ifdef BXR_DEBUG_LINKS
                sub_iter->parent = &*iter;
#endif
                if(-1 == sub_iter->next) {
                    break;
                }
                ++sub_iter;
            }
        }
    }

    for(auto& entry : m_main_items)
    {
        auto& parent = m_main_tags[ entry.data_tag ];
#ifdef BXR_DEBUG_LINKS
        parent.children.push_back(&entry);
#endif
        entry.tag_view = parent.symbol;
        if( -1 != entry.offset ) {
            entry.symbol.assign(&symbol.data()[ entry.offset ]);
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
    for(auto& entry: m_sub_items)
    {
        auto& parent = m_sub_tags[ entry.data_tag ];
#ifdef BXR_DEBUG_LINKS
        parent.children.push_back(&entry);
#endif
        entry.tag_view = parent.symbol;
        entry.symbol = std::string(&symbol.data()[ entry.offset ]);
    }

    if(!m_sub_tags.empty()) {
        m_property_name = m_sub_tags.front().symbol;
    }

    qInfo() << "filesize = " << std::filesystem::file_size(filepath);
    qInfo() << "calculated size = " << calculateSize();

    return {true, "File succesfully loaded."};
}

std::pair<bool, std::string> BXR::writeXML(std::filesystem::path const &filepath)
{
    QFile file(filepath);
    if(!file.open(QIODevice::WriteOnly)){
        return {false, ""};
    }
    QXmlStreamWriter xml_writer(&file);
    xml_writer.setAutoFormatting(true);
    std::vector<std::pair<MainScriptEntry*,int>> base_entry_list;
    for(auto& entry: m_main_items) {
        base_entry_list.emplace_back(&entry, 0);
    }

    for(auto iter = base_entry_list.begin(); iter != base_entry_list.end(); ++iter) {
        if (-1 != iter->first->next_ticks) {
            std::for_each(iter + 1, base_entry_list.begin() + iter->first->next_ticks, [](auto& pair){
                ++pair.second;
            });
        }
    }

    for (auto iter = base_entry_list.begin(); iter != base_entry_list.end(); ++iter) {
        auto const &[entry, power] = *iter;
        xml_writer.writeStartElement(QString::fromStdString(std::string(entry->tag_view)));
        if (-1 != entry->offset) {
            xml_writer.writeAttribute(m_property_name, entry->symbol);
        }
        if (-1 != entry->offset_unicode) {
            xml_writer.writeAttribute(unicode_literal, entry->unicode);
        }
        for (auto child : entry->subscript_children) {
            if (child->symbol.empty()) {
                xml_writer.writeStartElement(child->tag_view);
                xml_writer.writeAttribute(type_hack_literal, "sub");
                xml_writer.writeEndElement();
            } else {
                xml_writer.writeTextElement(child->tag_view, child->symbol);
            }
        }
        //Compare power of the next element to understand when it's time to step out
        if (auto const next = iter + 1; next != base_entry_list.end()) {
            if (auto const power_diff = power - next->second; power_diff >= 0) {
                xml_writer.writeEndElement();
                for (int d = 0; d < power_diff; ++d) {
                    xml_writer.writeEndElement();
                }
            }
        }
    }
    xml_writer.writeEndDocument();    
    return {true, ""};
}

std::pair<bool, std::string> BXR::readXML(std::filesystem::path const& filepath)
{
    auto const processAtributes = [this](QXmlStreamAttributes const& attributes)
        -> std::tuple<std::string, std::u16string, CandidateType> {
      auto type = CandidateType::main;
      if (attributes.empty()) {
        return {{}, {}, type};
      }
      std::string value;
      std::u16string unicode;

      for (auto const& attribute : attributes) {
        if (QString(unicode_literal) == attribute.name()) {
          unicode = attribute.value().toString().toStdU16String();
          continue;
        }
        if (QString(type_hack_literal) == attribute.name()) {
          if (attribute.value() == QString("sub")) {
            type = CandidateType::sub;
          }
          /*if(attribute.value() == QString("main")){
            type = CandidateType::main;
          }*/
          continue;
        }
        m_property_name = attribute.name().toString().toStdString();
        value = attribute.value().toString().toStdString();
      }
      return {value, unicode, type};
    };
    reset();
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {false, "Unable to open the file."};
    }
    QXmlStreamReader xml_reader(&file);

    std::vector<Candidate> candidates;

    int power = -1;
    while (!xml_reader.atEnd()) {
       switch(xml_reader.readNext()) {
       case QXmlStreamReader::StartDocument:
            break;
       case QXmlStreamReader::EndDocument:
            break;
       case QXmlStreamReader::StartElement:
       {
            ++power;
            auto const [value, unicode, type] = processAtributes(xml_reader.attributes());
            candidates.emplace_back(Candidate{.tag = xml_reader.name().toString().toStdString(), .value = value, .unicode = unicode, .power = power, .type = type});
       }
            break;
       case QXmlStreamReader::EndElement:
            --power;
            break;
       case QXmlStreamReader::Characters:
            if (xml_reader.text().startsWith('\n')) {
                break;
            } else {
                if (candidates.empty()) {
                    xml_reader.raiseError("Wrong sequence. Got characters with an empty stack.");
                    break;
                }
                candidates.back().type = CandidateType::sub;
                candidates.back().value = xml_reader.text().toString().toStdString();
            }
            break;
       default:
            break;
       }
    }
    if (xml_reader.hasError()) {
       return {false,
               "XML formatting error: " + xml_reader.errorString().toStdString()};
    }
    auto const main_items = candidates | adaptor::filtered(filterCandidateType<CandidateType::main>);
    auto const sub_items = candidates | adaptor::filtered(filterCandidateType<CandidateType::sub>);
    //Collect tags
    int32_t offset_counter = 0; //We're gonna enumerate offsets in [main tag, sub tag, entries] order.
    auto const addTag = [&offset_counter](Candidate const& source, std::vector<Offsetable>& taglist){
      if(auto const res = std::ranges::find_if(taglist, [&source](Offsetable const& entry){
        return source.tag == entry.symbol;
          }); res == taglist.end()) {
        auto const offset = offset_counter++;
        taglist.push_back(Offsetable{.offset = offset, .symbol = source.tag});
      }
    };
    for(auto const& item: main_items) {
       addTag(item, m_main_tags);
    }
    m_sub_tags.emplace_back(offset_counter++, m_property_name); //Add property name as our first subtag
    for(auto const& item: sub_items) {
       addTag(item, m_sub_tags);
    }
    //The crazy memory manipulation requires to prepare vectors
    //so they wouldn't invalidate pointers when reallocating themselves
    auto const main_items_size = std::distance(main_items.begin(), main_items.end());
    auto const sub_items_size = candidates.size() - main_items_size;
    m_main_items.reserve(main_items_size);
    m_sub_items.reserve(sub_items_size);
    //Collect items
    auto const match_tag = [](std::vector<Offsetable> const& taglist, std::string const& tag, OffsetTaggable* entry){
      auto const res = std::ranges::find_if(taglist, [&tag](Offsetable const& entry){
        return tag == entry.symbol;
      });
        entry->data_tag = std::distance(taglist.begin(), res);
        entry->tag_view = res->symbol;
    };
    for (auto const& item : candidates) {
         if (item.type == CandidateType::main) {
            auto& entry = m_main_items.emplace_back();
            match_tag(m_main_tags, item.tag, &entry);
            if(!item.value.empty()){
              entry.symbol = item.value;
              entry.offset = offset_counter++;
            }

            entry.power = item.power;
            if(!item.unicode.empty()) {
                entry.unicode = item.unicode;
                entry.offset_unicode = 1;
            }
         } else {
            auto& entry = m_sub_items.emplace_back();
            match_tag(m_sub_tags, item.tag, &entry);
            entry.symbol = item.value;
            entry.offset = offset_counter++;

            //Being a sub means that the entry is a child of a previous main entry
            auto &parent = m_main_items.back();
            if(parent.subscript_children.empty()) {
              parent.index_sub_item = m_sub_items.size() - 1;
            }
            parent.subscript_children.push_back(&entry);
            //We will process child-list later
         }
    }
    //Enumerate entries
    {
      int index = 0;
      std::for_each(m_main_items.begin(), m_main_items.end(), [&index](MainScriptEntry& child){
        child.before = index - 1;
        child.next = index + 1;
        ++index;
      });
      m_main_items.back().next = -1;
    }
    //Build tag hierarchy
    {
        auto iter = m_main_items.begin();
        while(iter != m_main_items.end()) {
            if(auto const next = iter + 1; next != m_main_items.end()) {
              if(next->power < iter->power){
                    iter->next_ticks = std::distance(m_main_items.begin(), next);
                    ++iter;
                    continue;
              }
            }
            auto const closure = std::find_if(iter + 1, m_main_items.end(), [&iter](MainScriptEntry& entry){
                return entry.power == iter->power;
            });
            iter->next_ticks = std::distance(m_main_items.begin(), closure);
            ++iter;
        }
    }
    //Enumerate children
    for (auto& item: m_main_items | adaptor::filtered([](MainScriptEntry const& entry){
                        return !entry.subscript_children.empty();
                      })) {
         int index = item.index_sub_item;
         std::for_each(item.subscript_children.begin(), item.subscript_children.end() - 1, [&index](SubScriptEntry* child){
           child->next = ++index;
         });
         item.subscript_children.back()->next = -1;
    }
    return {true,""};
}

uint32_t BXR::getUnicodeSize() const {
    return std::accumulate(
        m_main_items.begin(), m_main_items.end(), 0,
        [](const auto& a, const imas::file::BXR::MainScriptEntry& b) {
          return a + (-1 == b.offset_unicode ? 0 : (b.unicode.size() + 1) * 2);
        });
}

uint32_t BXR::calculateStringSize() const {
    auto string_count = getStringSize(m_main_tags) + getStringSize(m_sub_tags)
                        + getStringSize(m_main_items)
                        + getStringSize(m_sub_items);
    if (string_count % 2) {
       ++string_count;
    }
    return string_count + getUnicodeSize();
}

uint32_t BXR::calculateSize() const
{
    uint32_t const string_size = calculateStringSize();
    uint32_t label = 4;                     //label
    uint32_t offsets = (5 * 4);             //offsets
    uint32_t tag_m = (4 * m_main_tags.size()); //Tag data (1 param = 8 bit)
    uint32_t tag_s = (4 * m_sub_tags.size());  //
    uint32_t main = (4 * 7 * m_main_items.size());
    uint32_t sub = (4 * 3 * m_sub_items.size());
    return label + offsets + tag_m + tag_s + main + sub + string_size;
}

void BXR::setUnicodeData(std::vector<char>& output) {
    for(auto& entry: m_main_items) {
        if( -1 != entry.offset_unicode) {
            entry.offset_unicode = output.size();
            for (auto &sym : entry.unicode) {
                output.push_back(static_cast<char>((sym >> 8) & 0xFF));
                output.push_back(static_cast<char>(sym & 0xFF));
            }
            output.push_back('\0');
            output.push_back('\0');
        }
    }
}

template<class T>
void BXR::insertIntoQueue(std::vector<T>& origin, std::vector<imas::file::BXR::Offsetable*>& output) {
    auto transformed_range = origin | adaptor::filtered([](T& value){
                                 return -1 != value.offset;
                             }) | adaptor::transformed([](T& value){
                                   return &value;
                               });
    output.insert(output.end(), transformed_range.begin(), transformed_range.end());
}

std::pair<bool, std::string> BXR::save(std::filesystem::path const& filepath) {
    std::ofstream stream(filepath, std::ios_base::binary);

    // 1. Let's start building the string chunk
    std::vector<Offsetable*> offset_queue;

    insertIntoQueue(m_main_tags, offset_queue);
    insertIntoQueue(m_sub_tags, offset_queue);
    insertIntoQueue(m_main_items, offset_queue);
    insertIntoQueue(m_sub_items, offset_queue);

    std::ranges::sort(offset_queue,
                      [](Offsetable const* left, Offsetable const* right) {
                        return left->offset < right->offset;
                      });

    std::vector<char> string_library;
    string_library.reserve(calculateStringSize());
    setStringData(offset_queue, string_library);
    // Even the stream before writing unicode part
    if (string_library.size() % 2) {
        string_library.push_back('\0');
    }
    setUnicodeData(string_library);

    // Write label
    std::string label{bxr_label};
    stream.write(label.data(), label.size());

    // Write base offsets
    imas::tools::writeLong(stream, m_main_tags.size());
    imas::tools::writeLong(stream, m_sub_tags.size());
    imas::tools::writeLong(stream, m_main_items.size());
    imas::tools::writeLong(stream, m_sub_items.size());
    imas::tools::writeLong(stream, string_library.size());

    // Write sections data
    for (auto& entry : m_main_tags) {
        imas::tools::writeLong(stream, entry.offset);
    }

    // BLOCK2
    // And a subscript tag offset
    for (auto& entry : m_sub_tags) {
        imas::tools::writeLong(stream, entry.offset);
    }

    // BLOCK3
    // We fill the mainscript entries with the integer data
    for (auto& entry : m_main_items) {
        imas::tools::writeLong(stream, entry.before);
        imas::tools::writeLong(stream, entry.next);
        imas::tools::writeLong(stream, entry.data_tag);
        imas::tools::writeLong(stream, entry.offset);
        imas::tools::writeLong(stream, entry.index_sub_item);
        imas::tools::writeLong(stream, entry.offset_unicode);
        imas::tools::writeLong(stream, entry.next_ticks);
    }

    // BLOCK4
    // Ditto with subscript
    for (auto& entry : m_sub_items) {
        imas::tools::writeLong(stream, entry.next);
        imas::tools::writeLong(stream, entry.data_tag);
        imas::tools::writeLong(stream, entry.offset);
    }

    //. Write string chunk
    stream.write(string_library.data(), string_library.size());
    return {true, ""};
}

void BXR::reset() {
    m_main_tags.clear();
    m_sub_tags.clear();
    m_main_items.clear();
    m_sub_items.clear();
    m_property_name.clear();
}

//QJsonArray BXR::getJson()
//{
//    QJsonArray BXR_json;
//    for (auto const &entry : unicode_containing_entries) {
//        BXR_json.append(QString::fromStdU16String(entry.get().unicode));
//    }
//    return BXR_json;
//}

//std::pair<bool, std::string> BXR::setJson(const QJsonValue& json)
//{
//    auto const array = json.toArray();
//    if (array.size() != unicode_containing_entries.size()) {
//        return {false, "BXR string injection error: Count mismatch. Expected " + std::to_string(unicode_containing_entries.size()) + ", got " + std::to_string(array.size()) + ". Are you loading the correct file?"};
//    }
//
//    for (size_t ind = 0; ind < unicode_containing_entries.size(); ++ind) {
//        unicode_containing_entries[ind].get().unicode = array.at(ind).toString().toStdU16String();
//    }
//    return {true, ""};
//}

}
}
