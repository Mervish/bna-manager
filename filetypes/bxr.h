#pragma once

#include "utility/stdhacks.h"
#include "utility/streamtools.h"

#include <vector>
#include <string>
#include <fstream>

namespace imas{
namespace file{

class BXR
{
public:
    template<class T>
    struct OffsetData
	{
        int32_t offset_symbol;
        std::string symbol;
        std::vector<T*> children;
	};

    /*struct Offset {
        OffsetData tag;
        OffsetData data;
    };*/

    struct SubScriptEntry
    {
        int data_tag;	// tag_subScriptへ
        int next;
        int offset_symbol;

        std::string_view sub_symbol;
        std::string symbol;
    };

    struct MainScriptEntry
    {
        int before;
        int data_tag;
        int index_sub_script;
        int next;
        int nextTicks;
        int offset_symbol;		// 数値/camera_a/face/motion/part/song/event等
        int offset_unicode;		// 歌詞

        std::string_view main_symbol;		// dataTag
        std::string sub_symbol;			// offset_symbol
        std::u16string unicode;			// 歌詞

        std::vector<MainScriptEntry*> children;
        std::vector<SubScriptEntry*> subscript_children;
    };

    struct Offsets {
        unsigned int tag_main_script;
        unsigned int tag_sub_script;
        unsigned int main_script;
        unsigned int sub_script;
        unsigned int symbol_offset;
    };

    std::vector<OffsetData<MainScriptEntry>> tag_mainScript; // ダンス内の場合はroot/beat/data/start/words/camera/faces
    std::vector<OffsetData<SubScriptEntry>> tag_subScript;  // ダンス内の場合はparam*/main_vocal_part/start_dance/start_title/wait/font/position_start/scale_x/rate* 等
    std::vector<MainScriptEntry> mainScript;
    std::vector<SubScriptEntry> subScript;
    MainScriptEntry* root;
public:

    bool Load( std::string filename )
	{
        std::ifstream stream(filename, std::ios_base::binary);

        std::string label;
        label.resize(4);
        stream.read(label.data(), 4);

        if("BXR0" != label) {
            return false;
        }

        Offsets offsets;

        //First - we read offset array sizes
        offsets.tag_main_script = imas::tools::readLong(stream);
        offsets.tag_sub_script  = imas::tools::readLong(stream);
        offsets.main_script     = imas::tools::readLong(stream);
        offsets.sub_script      = imas::tools::readLong(stream);
        offsets.symbol_offset   = imas::tools::readLong(stream);

        // BLOCK1
        // Then we read main script tag offset
        tag_mainScript.resize( offsets.tag_main_script );
        for(auto& entry: tag_mainScript) {
            entry.offset_symbol = imas::tools::readLong(stream);
        }

        // BLOCK2
        // And a subscript tag offset
        tag_subScript.resize( offsets.tag_sub_script );
        for(auto& entry: tag_subScript) {
            entry.offset_symbol = imas::tools::readLong(stream);
        }

		// BLOCK3
        // We fill the mainscript entries with the integer data
        mainScript.resize( offsets.main_script );
        for(auto& entry: mainScript)
		{
            entry.before           = imas::tools::readULong(stream);
            entry.next             = imas::tools::readULong(stream);
            entry.data_tag         = imas::tools::readULong(stream);
            entry.offset_symbol    = imas::tools::readULong(stream);
            entry.index_sub_script = imas::tools::readULong(stream);
            entry.offset_unicode   = imas::tools::readULong(stream);
            entry.nextTicks   = imas::tools::readULong(stream);
		}

		// BLOCK4
        // Ditto with subscript
        subScript.resize( offsets.sub_script );
        for(auto& entry: subScript)
		{
            entry.next         = imas::tools::readULong(stream);
            entry.data_tag      = imas::tools::readULong(stream);
            entry.offset_symbol = imas::tools::readULong(stream);
		}

		// BLOCK5
        // Then we read some sort of symbolic data
        std::vector<char> symbol(offsets.symbol_offset);
        stream.read(symbol.data(), symbol.size());

		//
		// 解釈
		//

        // BLOCK1
        // Then we fill symbol data for the tag and script sections
        for(auto& entry : tag_mainScript) {
            entry.symbol.assign(&symbol.data()[ entry.offset_symbol ]);
        }

        // BLOCK2
        for(auto& entry : tag_subScript) {
            entry.symbol.assign(&symbol.data()[ entry.offset_symbol ]);
        }

        // BLOCK3
        root = &mainScript.front();
        for(auto iter = mainScript.begin(); iter != mainScript.end();++iter) {
            if( -1 != iter->nextTicks) {
                if( -1 != iter->index_sub_script) {
                    auto const sub_iter = subScript.begin() + iter->index_sub_script;
                    std::for_each_n(sub_iter, iter->nextTicks, [this, &iter](SubScriptEntry& entry){
                        iter->subscript_children.push_back(&entry);
                    });
                }else{
                    std::for_each_n(iter + 1, iter->nextTicks - 1, [this, &iter](MainScriptEntry& entry){
                        iter->children.push_back(&entry);
                    });
                }
            }
        }

        /*for(size_t ind = 0; ind < mainScript.size(); ++ind) {
            auto& entry = mainScript[ind];
            if( -1 != entry.children_count) {
                for(size_t subind = 0; subind < entry.children_count; ++subind) {
                    entry.children.push_back(&mainScript[subind]);
                }
            }
        }*/

        for(auto& entry : mainScript)
		{
            auto& parent = tag_mainScript[ entry.data_tag ];
            parent.children.push_back(&entry);
            entry.main_symbol = parent.symbol;
            if( -1 != entry.offset_symbol ) {
                entry.sub_symbol.assign(&symbol.data()[ entry.offset_symbol ]);
            }

			// 歌詞など
            if( -1 != entry.offset_unicode )
			{
                entry.unicode.assign( (char16_t*) & symbol[ entry.offset_unicode ]);
                //Curiously, symbol data has mixed 8 and 16 bit-wide strings
                //So we need to convert the endianess on the fly
                for(auto& character: entry.unicode) {
                    character = byteswap(character);
                }
			}
		}

		// BLOCK4
        for(auto& entry: subScript)
		{
            auto& parent = tag_subScript[ entry.data_tag ];
            parent.children.push_back(&entry);
            entry.sub_symbol = parent.symbol;
            entry.symbol = std::string(&symbol.data()[ entry.offset_symbol ]);
		}

        //So, potential save alghoritm may look like this:
        //1. Calculate the general offset based on the cound of the entries(it's actually will remain unchanged from the original)
        //2. Go over all of the sections and gather the string data
        //3. Create a string chunk and fill the offset data (basically, position before writing the string is your offset)(offset data is relative to the string chunk)
        //4. Start writing the file
        //5. Write count(should be unchanged due to modification)
        //6. Write int data of each section, should end up taking the same space
        //7. Write string chunk

		return true;
	}
};

}   // namespace file
}	// namespace imas
