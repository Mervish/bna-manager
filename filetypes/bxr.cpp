#include "bxr.h"

#include "utility/streamtools.h"
#include "utility/stringtools.h"

#include <iostream>
#include <numeric>
#include <ranges>

#include <boost/range/adaptors.hpp>

namespace adaptor = boost::adaptors;

namespace {
constexpr auto bxr_label = "BXR0";
constexpr auto unicode_literal = "unicode";
constexpr auto type_hack_literal = "type";
constexpr auto type_sub_literal = "sub";

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

struct Candidate;

typedef std::shared_ptr<Candidate> CandidatePtr;

struct Candidate {
    std::string tag;
    std::string value;
    std::u16string unicode;
    int power;
    CandidateType type = CandidateType::main;
    std::vector<CandidatePtr> main_children;
    std::vector<CandidatePtr> sub_children;
};

template <CandidateType F>
bool filterCandidateType(Candidate const& entry) {
    return F == entry.type;
}

//makes hierarchy based on range
template <class R>
std::vector<imas::file::BXR::MainScriptEntry*> makeChildrenHierarchy(R const& range, int cur_tick) {
  std::vector<imas::file::BXR::MainScriptEntry*> res;
  for(auto iter = range.begin();iter != range.end();) {
    res.push_back(*iter);
    auto const ticks = (*iter)->next_ticks - cur_tick;
    if(ticks > 1) {
      (*iter)->children = makeChildrenHierarchy(std::ranges::subrange(iter + 1, iter + ticks), cur_tick + 1);
    }
    iter += ticks;
    cur_tick += ticks;
  }
  return res;
};
}

namespace imas{
namespace file{

Manageable::Fileapi BXR::api() const {
  static auto const api = Fileapi{.base_extension = "bxr",
                          .final_extension = "xml",
                          .type = ExtractType::file,
                          .signature = "XML file (*.xml)",
                          .extraction_title = "Export as XML...",
                          .injection_title = "Import from XML..."};
  return api;
}

Result BXR::openFromStream(std::basic_istream<char> *stream)
{
    std::string label;
    label.resize(4);
    stream->read(label.data(), 4);

    if(bxr_label != label) {
        return {false, "wrong filetype, file label mismatch"};
    }
    reset();

    SectionSizes sizes;

    //First - we read offset array sizes
    sizes.tag_main_script = imas::utility::readLong(stream);
    sizes.tag_sub_script  = imas::utility::readLong(stream);
    sizes.main_script     = imas::utility::readLong(stream);
    sizes.sub_script      = imas::utility::readLong(stream);
    sizes.symbol_offset   = imas::utility::readLong(stream);

    // BLOCK1
    // Then we read main script tag offset
    m_main_tags.resize( sizes.tag_main_script );
    for(auto& entry: m_main_tags) {
        entry.offset = imas::utility::readLong(stream);
    }

    // BLOCK2
    // And a subscript tag offset
    m_sub_tags.resize( sizes.tag_sub_script );
    for(auto& entry: m_sub_tags) {
        entry.offset = imas::utility::readLong(stream);
    }

    // BLOCK3
    // We fill the mainscript entries with the integer data
    m_main_items.resize( sizes.main_script );
    for(auto& entry: m_main_items)
    {
        entry.before           = imas::utility::readLong(stream);
        entry.next             = imas::utility::readLong(stream);
        entry.data_tag         = imas::utility::readLong(stream);
        entry.offset         = imas::utility::readLong(stream);
        entry.index_sub_item = imas::utility::readLong(stream);
        entry.offset_unicode   = imas::utility::readLong(stream);
        entry.next_ticks   = imas::utility::readLong(stream);
    }

    // BLOCK4
    // Ditto with subscript
    m_sub_items.resize( sizes.sub_script );
    for(auto& entry: m_sub_items)
    {
        entry.next         = imas::utility::readLong(stream);
        entry.data_tag      = imas::utility::readLong(stream);
        entry.offset = imas::utility::readLong(stream);
    }

    // BLOCK5
    // Then we read some sort of symbolic data
    std::vector<char> symbol(sizes.symbol_offset);
    stream->read(symbol.data(), symbol.size());

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
    for(auto iter = m_main_items.begin();iter != m_main_items.end();++iter) {
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

    m_root = makeChildrenHierarchy(m_main_items | std::views::transform([](MainScriptEntry& entry){
                              return &entry;
                            }), 0);

    // for(auto iter = m_main_items.begin();iter != m_main_items.end();++iter) {
    //   m_root.push_back(&*iter);
    //   auto const ticks = iter->next_ticks;
    //   auto const children_range = std::ranges::subrange(iter + 1, iter + ticks) | std::views::transform([](MainScriptEntry& entry){
    //     return &entry;
    //   });
    //   iter->children.assign(children_range.begin(), children_range.end());
    //   iter += ticks;
    // }
    //next ticks is amount of children
    //you can get it from xml by literally counting children
    //I only need to "unwrap" all of the children into the list
    //probably should make methods for setting next_ticks recursively or unwrapping children

    for(auto& entry : m_main_items)
    {
        auto& tag_data = m_main_tags[ entry.data_tag ];
#ifdef BXR_DEBUG_LINKS
        tag_data.entries.push_back(&entry);
#endif
        entry.tag_view = tag_data.symbol;
        if( -1 != entry.offset ) {
            entry.symbol.assign(&symbol.data()[ entry.offset ]);
        }

        // 歌詞など
        if( -1 != entry.offset_unicode )
        {
            entry.unicode.assign( (char16_t*) & symbol[ entry.offset_unicode ]);
            //Curiously, symbol data has mixed 8 and 16 bit-wide strings
            //So we need to convert the endianess for the 16 bit strings
            for(auto& character: entry.unicode) {
                character = std::byteswap(character);
            }
        }
    }

    // BLOCK4
    for(auto& entry: m_sub_items)
    {
        auto& tag_data = m_sub_tags[ entry.data_tag ];
#ifdef BXR_DEBUG_LINKS
        tag_data.entries.push_back(&entry);
#endif
        entry.tag_view = tag_data.symbol;
        entry.symbol = std::string(&symbol.data()[ entry.offset ]);
    }

    if(auto const prop_i = std::ranges::find(m_sub_tags, 0, [](auto const& entry){
        return entry.entries.size();
    }); prop_i != m_sub_tags.end()) {
      m_property_name = prop_i->symbol;
    }else{
      return {false, "failed to find property name"};
    }

    return {true, "succesfully loaded"};
}

// Result BXR::extract(std::filesystem::path const &savepath) const
// {
//     QFile file(savepath);
//     if(!file.open(QIODevice::WriteOnly)){
//         return {false, "unable to open the file"};
//     }
//     QXmlStreamWriter xml_writer(&file);
//     xml_writer.setAutoFormatting(true);
//     std::vector<std::pair<const MainScriptEntry*,int>> base_entry_list;
//     for(auto& entry: m_main_items) {
//         base_entry_list.emplace_back(&entry, 0);
//     }

//     for(auto iter = base_entry_list.begin(); iter != base_entry_list.end(); ++iter) {
//         if (-1 != iter->first->next_ticks) {
//             std::for_each(iter + 1, base_entry_list.begin() + iter->first->next_ticks, [](auto& pair){
//                 ++pair.second;
//             });
//         }
//     }

//     for (auto iter = base_entry_list.begin(); iter != base_entry_list.end(); ++iter) {
//         auto const &[entry, power] = *iter;
//         xml_writer.writeStartElement(QString::fromStdString(std::string(entry->tag_view)));
//         if (-1 != entry->offset) {
//             xml_writer.writeAttribute(m_property_name, entry->symbol);
//         }
//         if (-1 != entry->offset_unicode) {
//             xml_writer.writeAttribute(unicode_literal, entry->unicode);
//         }
//         for (auto child : entry->subscript_children) {
//             if (child->symbol.empty()) {
//                 xml_writer.writeStartElement(child->tag_view);
//                 xml_writer.writeAttribute(type_hack_literal, type_sub_literal);

//                 xml_writer.writeEndElement();
//             } else {
//                 xml_writer.writeTextElement(child->tag_view, child->symbol);
//             }
//         }
//         //Compare power of the next element to understand when it's time to step out
//         if (auto const next = iter + 1; next != base_entry_list.end()) {
//             if (auto const power_diff = power - next->second; power_diff >= 0) {
//                 xml_writer.writeEndElement();
//                 for (int d = 0; d < power_diff; ++d) {
//                     xml_writer.writeEndElement();
//                 }
//             }
//         }
//     }
//     xml_writer.writeEndDocument();
//     return {true, ""};
// }

void BXR::SubScriptEntry::setNode(pugi::xml_node &parent) const {
  auto node = parent.append_child(pugi::xml_node_type::node_element);
  node.set_name(tag_view.data());
  auto text = node.append_child(pugi::xml_node_type::node_pcdata);
  text.set_value(symbol.c_str());
}

void BXR::MainScriptEntry::setNode(pugi::xml_node &parent, std::string const& property_name) const {
  auto node = parent.append_child(pugi::xml_node_type::node_element);

  node.set_name(tag_view.data());

  if (-1 != offset) {
    auto attr = node.append_attribute(property_name.c_str());
    attr.set_value(symbol.c_str());
  }

  if(-1 != offset_unicode) {
    WidestringConv converter;
    auto uni_attr = node.append_attribute(unicode_literal);
    uni_attr.set_value(converter.to_bytes(unicode).c_str());
  }

  for(auto const& child: children) {
    child->setNode(node, property_name);
  }

  for(auto const& sub_child: subscript_children) {
    sub_child->setNode(node);
  }
}

Result BXR::extract(std::filesystem::path const &savepath) const
{
  pugi::xml_document doc;

  for(auto const& root_elem: m_root) {
    root_elem->setNode(doc, m_property_name);
  }

  doc.save_file(savepath.string().c_str());
  return {true, ""};
}

// Result BXR::inject(std::filesystem::path const& openpath)
// {
//     auto const processAtributes = [this](QXmlStreamAttributes const& attributes)
//         -> std::tuple<std::string, std::u16string, CandidateType> {
//       auto type = CandidateType::main;
//       if (attributes.empty()) {
//         return {{}, {}, type};
//       }
//       std::string value;
//       std::u16string unicode;

//       for (auto const& attribute : attributes) {
//         if (QString(unicode_literal) == attribute.name()) {
//           unicode = attribute.value().toString().toStdU16String();
//           continue;
//         }
//         if (QString(type_hack_literal) == attribute.name()) {
//           if (attribute.value() == QString(type_sub_literal)) {
//             type = CandidateType::sub;
//           }
//           /*if(attribute.value() == QString("main")){
//             type = CandidateType::main;
//           }*/
//           continue;
//         }
//         m_property_name = attribute.name().toString().toStdString();
//         value = attribute.value().toString().toStdString();
//       }
//       return {value, unicode, type};
//     };
//     reset();
//     QFile file(openpath);
//     if (!file.open(QIODevice::ReadOnly)) {
//         return {false, "unable to open the file"};
//     }
//     QXmlStreamReader xml_reader(&file);

//     std::vector<Candidate> candidates;

//     int power = -1;
//     while (!xml_reader.atEnd()) {
//        switch(xml_reader.readNext()) {
//        case QXmlStreamReader::StartDocument:
//             break;
//        case QXmlStreamReader::EndDocument:
//             break;
//        case QXmlStreamReader::StartElement:
//        {
//             ++power;
//             auto const [value, unicode, type] = processAtributes(xml_reader.attributes());
//             candidates.emplace_back(Candidate{.tag = xml_reader.name().toString().toStdString(), .value = value, .unicode = unicode, .power = power, .type = type});
//        }
//             break;
//        case QXmlStreamReader::EndElement:
//             //Instead of using hack, we can
//             --power;
//             break;
//        case QXmlStreamReader::Characters:
//             if (xml_reader.text().startsWith('\n')) {
//                 break;
//             } else {
//                 if (candidates.empty()) {
//                     xml_reader.raiseError("Wrong sequence. Got characters with an empty stack.");
//                     break;
//                 }
//                 candidates.back().type = CandidateType::sub;
//                 candidates.back().value = xml_reader.text().toString().toStdString();
//             }
//             break;
//        default:
//             break;
//        }
//     }
//     if (xml_reader.hasError()) {
//        return {false,
//                "XML formatting error: " + xml_reader.errorString().toStdString()};
//     }
//     auto const main_items = candidates | adaptor::filtered(filterCandidateType<CandidateType::main>);
//     auto const sub_items = candidates | adaptor::filtered(filterCandidateType<CandidateType::sub>);
//     //Collect tags
//     int32_t offset_counter = 0; //We're gonna enumerate offsets in [main tag, sub tag, entries] order.
//     auto const addTag = [&offset_counter](Candidate const& source, auto& taglist){
//       if(auto const res = std::ranges::find_if(taglist, [&source](auto const& entry){
//         return source.tag == entry.symbol;
//           }); res == taglist.end()) {
//         auto const offset = offset_counter++;
//         typename std::remove_reference<decltype(taglist)>::type::value_type entry;
//         entry.offset = offset;
//         entry.symbol = source.tag;
//         taglist.push_back(entry);
//       }
//     };
//     for(auto const& item: main_items) {
//        addTag(item, m_main_tags);
//     }
//     decltype(m_sub_tags)::value_type subentry;
//     subentry.offset = offset_counter++;
//     subentry.symbol = m_property_name;
//     m_sub_tags.emplace_back(subentry); //Add property name as our first subtag
//     for(auto const& item: sub_items) {
//        addTag(item, m_sub_tags);
//     }
//     //The memory manipulation required to prepare vectors
//     //so they wouldn't invalidate pointers when reallocating themselves
//     auto const main_items_size = std::ranges::distance(main_items);
//     auto const sub_items_size = candidates.size() - main_items_size;
//     m_main_items.reserve(main_items_size);
//     m_sub_items.reserve(sub_items_size);
//     //Collect items
//     auto const match_tag = [](auto const& taglist, std::string const& tag, OffsetTaggable* entry){
//       auto const res = std::ranges::find_if(taglist, [&tag](Offsetable const& entry){
//         return tag == entry.symbol;
//       });
//         entry->data_tag = std::distance(taglist.begin(), res);
//         entry->tag_view = res->symbol;
//     };
//     for (auto const& item : candidates) {
//          if (item.type == CandidateType::main) {
//             auto& entry = m_main_items.emplace_back();
//             match_tag(m_main_tags, item.tag, &entry);
//             if(!item.value.empty()){
//               entry.symbol = item.value;
//               entry.offset = offset_counter++;
//             }

//             entry.power = item.power;
//             if(!item.unicode.empty()) {
//                 entry.unicode = item.unicode;
//                 entry.offset_unicode = 1;
//             }
//          } else {
//             auto& entry = m_sub_items.emplace_back();
//             match_tag(m_sub_tags, item.tag, &entry);
//             entry.symbol = item.value;
//             entry.offset = offset_counter++;

//             //Being a sub means that the entry is a child of a previous main entry
//             auto &parent = m_main_items.back();
//             if(parent.subscript_children.empty()) {
//               parent.index_sub_item = m_sub_items.size() - 1;
//             }
//             parent.subscript_children.push_back(&entry);
//             //We will process child-list later
//          }
//     }
//     //Enumerate entries
//     {
//       int index = 0;
//       std::for_each(m_main_items.begin(), m_main_items.end(), [&index](MainScriptEntry& child){
//         child.before = index - 1;
//         child.next = index + 1;
//         ++index;
//       });
//       m_main_items.back().next = -1;
//     }
//     //Build tag hierarchy
//     {
//         auto iter = m_main_items.begin();
//         while(iter != m_main_items.end()) {
//             if(auto const next = iter + 1; next != m_main_items.end()) {
//               if(next->power < iter->power){
//                     iter->next_ticks = std::distance(m_main_items.begin(), next);
//                     ++iter;
//                     continue;
//               }
//             }
//             auto const closure = std::find_if(iter + 1, m_main_items.end(), [&iter](MainScriptEntry& entry){
//                 return entry.power == iter->power;
//             });
//             iter->next_ticks = std::distance(m_main_items.begin(), closure);
//             ++iter;
//         }
//     }
//     //Enumerate children
//     for (auto& item: m_main_items | adaptor::filtered([](MainScriptEntry const& entry) {
//                         return !entry.subscript_children.empty();
//                       })) {
//          int index = item.index_sub_item;
//          std::for_each(item.subscript_children.begin(), item.subscript_children.end() - 1, [&index](SubScriptEntry* child){
//            child->next = ++index;
//          });
//          item.subscript_children.back()->next = -1;
//     }
//     return {true,"succesfully imported from XML"};
// }

template<pugi::xml_node_type T>
bool checkNodeType(pugi::xml_node const& node) {
  return node.type() == T;
}

Result BXR::inject(std::filesystem::path const &openpath) {
  reset();
  pugi::xml_document doc;
  doc.load_file(openpath.string().c_str());

  std::vector<CandidatePtr> root_candidates;
  std::vector<CandidatePtr> main_items;
  std::vector<CandidatePtr> sub_items;

  WidestringConv conventer;

  std::function<void(CandidatePtr &, pugi::xml_node const &, int)> nodeWalker;
  nodeWalker = [&nodeWalker, &conventer, &main_items, &sub_items,
                this](CandidatePtr &parent, pugi::xml_node const &node,
                      int power) {
    parent->tag = node.name();
    parent->power = power;

    for (auto const &attr : node.attributes()) {
      if (0 == std::strcmp(unicode_literal, attr.name())) {
        parent->unicode = conventer.from_bytes(attr.value());
      } else {
        if (m_property_name.empty()) {
          m_property_name = attr.name();
        }
        parent->value = node.attributes_begin()->value();
      }
    }

    if (node.empty()) {
      parent->type = CandidateType::sub;
    }

    // parent.main_children.reserve(std::ranges::distance(node |
    // std::views::filter(checkNodeType<pugi::xml_node_type::node_element>)));
    // parent.sub_children.reserve(std::ranges::distance(node |
    // std::views::filter([](pugi::xml_node const& node){
    //   return pugi::xml_node_type::node_pcdata ==
    //   node.children().begin()->type();
    // })));
    for (auto const &n_child : node) {
      if (n_child.type() == pugi::xml_node_type::node_pcdata) {
        parent->type = CandidateType::sub;
        parent->value = n_child.value();
        break;
      }
      CandidatePtr c_child = std::make_shared<Candidate>();
      nodeWalker(c_child, n_child, power + 1);
      switch (c_child->type) {
      case CandidateType::main: {
        parent->main_children.push_back(c_child);
      } break;
      case CandidateType::sub: {
        parent->sub_children.push_back(c_child);
      } break;
      default:
        break;
      }
    }
  };
  //root_candidates.reserve(std::ranges::distance(doc));
  for (auto const &root_elem : doc) {
    auto &candidate = root_candidates.emplace_back(std::make_shared<Candidate>());
    main_items.push_back(candidate);
    nodeWalker(candidate, root_elem, 0);
  }
  std::function<void(CandidatePtr const&)> candidateUnwrapper;
  candidateUnwrapper = [&candidateUnwrapper, &main_items, &sub_items](CandidatePtr const& candidate) {
    for(auto const& sub: candidate->sub_children) {
      sub_items.push_back(sub);
    }
    for(auto const& main: candidate->main_children) {
      main_items.push_back(main);
      candidateUnwrapper(main);
    }
  };
  for(auto const& root_cand: root_candidates) {
    candidateUnwrapper(root_cand);
  }
  //
  m_main_items.reserve(main_items.size());
  m_sub_items.reserve(sub_items.size());
  // Collect tags
  int32_t offset_counter = 0;
  // We're gonna enumerate offsets in {main tag, sub tag, entries} order.
  auto const addTag = [&offset_counter](CandidatePtr const& source,
                                        auto &taglist) {
    if (auto const res = std::ranges::find_if(taglist,
                                              [&source](auto const &entry) {
                                                return source->tag ==
                                                       entry.symbol;
                                              });
        res == taglist.end()) {
      auto const offset = offset_counter++;
      typename std::remove_reference<decltype(taglist)>::type::value_type entry;
      entry.offset = offset;
      entry.symbol = source->tag;
      taglist.push_back(entry);
    }
  };
  for (auto const& item : main_items) {
    addTag(item, m_main_tags);
  }
  decltype(m_sub_tags)::value_type subentry;
  subentry.offset = offset_counter++;
  subentry.symbol = m_property_name;
  m_sub_tags.emplace_back(subentry); // Add property name as our first subtag
  for (auto const& item : sub_items) {
    addTag(item, m_sub_tags);
  }
  auto const match_tag = [](auto const &taglist, std::string const &tag,
                            OffsetTaggable *entry) {
    auto const res =
        std::ranges::find_if(taglist, [&tag](Offsetable const &entry) {
          return tag == entry.symbol;
        });
    entry->data_tag = std::distance(taglist.begin(), res);
    entry->tag_view = res->symbol;
  };
  std::function<void(CandidatePtr const&)> entryUnwrapper;
  auto const childUnwrapper = [&offset_counter, this, &match_tag](CandidatePtr const& c_ptr) {
    auto &entry = m_sub_items.emplace_back();
    match_tag(m_sub_tags, c_ptr->tag, &entry);
    entry.symbol = c_ptr->value;
    entry.offset = offset_counter++;

    // Being a sub means that the entry is a child of a previous main entry
    auto &parent = m_main_items.back();
    if (parent.subscript_children.empty()) {
      parent.index_sub_item = m_sub_items.size() - 1;
    }
    parent.subscript_children.push_back(&entry);
  };
  entryUnwrapper = [&offset_counter, &entryUnwrapper, this, &match_tag, &childUnwrapper](CandidatePtr const& c_ptr) {
    auto &entry = m_main_items.emplace_back();
    match_tag(m_main_tags, c_ptr->tag, &entry);
    if (!c_ptr->value.empty()) {
      entry.symbol = c_ptr->value;
      entry.offset = offset_counter++;
    }

    entry.power = c_ptr->power;
    if (!c_ptr->unicode.empty()) {
      entry.unicode = c_ptr->unicode;
      entry.offset_unicode = 1;
    }

    if(entry.power) {
      auto &parent = m_main_items.back();
      parent.children.push_back(&entry);
    }

    for(auto const& child: c_ptr->sub_children) {
      childUnwrapper(child);
    }
    for(auto const& child: c_ptr->main_children) {
      entryUnwrapper(child);
    }
  };
  for(auto& candidate: root_candidates) {
    entryUnwrapper(candidate);
  }
  // Enumerate entries
  {
    int index = 0;
    std::for_each(m_main_items.begin(), m_main_items.end(),
                  [&index](MainScriptEntry &child) {
                    child.before = index - 1;
                    child.next = index + 1;
                    ++index;
                  });
    m_main_items.back().next = -1;
  }
  // Build tag hierarchy
  {
    auto iter = m_main_items.begin();
    while (iter != m_main_items.end()) {
      if (auto const next = iter + 1; next != m_main_items.end()) {
        if (next->power < iter->power) {
          iter->next_ticks = std::distance(m_main_items.begin(), next);
          ++iter;
          continue;
        }
      }
      auto const closure = std::find_if(iter + 1, m_main_items.end(),
                                        [&iter](MainScriptEntry &entry) {
                                          return entry.power == iter->power;
                                        });
      iter->next_ticks = std::distance(m_main_items.begin(), closure);
      ++iter;
    }
  }
  // Enumerate children
  for (auto &item :
       m_main_items | std::views::filter([](MainScriptEntry const &entry) {
         return !entry.subscript_children.empty();
       })) {
    int index = item.index_sub_item;
    std::for_each(item.subscript_children.begin(),
                  item.subscript_children.end() - 1,
                  [&index](SubScriptEntry *child) { child->next = ++index; });
    item.subscript_children.back()->next = -1;
  }

  return {true, "xml data injected"};
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

size_t BXR::size() const {
    uint32_t const string_size = calculateStringSize();
    uint32_t label = 4;                        // label
    uint32_t offsets = (5 * 4);                // offsets
    uint32_t tag_m = (4 * m_main_tags.size()); // Tag data (1 param = 8 bit)
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

Result BXR::saveToStream(std::basic_ostream<char> *stream) {
    // 1. Let's start building the string chunk
    std::vector<Offsetable*> offset_queue;

    insertIntoQueue(m_main_tags, offset_queue);
    insertIntoQueue(m_sub_tags, offset_queue);
    insertIntoQueue(m_main_items, offset_queue);
    insertIntoQueue(m_sub_items, offset_queue);

    std::ranges::sort(offset_queue, {}, &Offsetable::offset);

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
    stream->write(label.data(), label.size());

    // Write base offsets
    imas::utility::writeLong(stream, m_main_tags.size());
    imas::utility::writeLong(stream, m_sub_tags.size());
    imas::utility::writeLong(stream, m_main_items.size());
    imas::utility::writeLong(stream, m_sub_items.size());
    imas::utility::writeLong(stream, string_library.size());

    // Write sections data
    for (auto& entry : m_main_tags) {
        imas::utility::writeLong(stream, entry.offset);
    }

    // BLOCK2
    // And a subscript tag offset
    for (auto& entry : m_sub_tags) {
        imas::utility::writeLong(stream, entry.offset);
    }

    // BLOCK3
    // We fill the mainscript entries with the integer data
    for (auto& entry : m_main_items) {
        imas::utility::writeLong(stream, entry.before);
        imas::utility::writeLong(stream, entry.next);
        imas::utility::writeLong(stream, entry.data_tag);
        imas::utility::writeLong(stream, entry.offset);
        imas::utility::writeLong(stream, entry.index_sub_item);
        imas::utility::writeLong(stream, entry.offset_unicode);
        imas::utility::writeLong(stream, entry.next_ticks);
    }

    // BLOCK4
    // Ditto with subscript
    for (auto& entry : m_sub_items) {
        imas::utility::writeLong(stream, entry.next);
        imas::utility::writeLong(stream, entry.data_tag);
        imas::utility::writeLong(stream, entry.offset);
    }

    //. Write string chunk
    stream->write(string_library.data(), string_library.size());
    return {true, ""};
}

void BXR::reset() {
    m_main_tags.clear();
    m_sub_tags.clear();
    m_main_items.clear();
    m_sub_items.clear();
    m_root.clear();
    m_property_name.clear();
}

}
}
