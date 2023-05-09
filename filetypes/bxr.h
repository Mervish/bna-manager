#pragma once

#include <numeric>
#include <vector>
#include <string>
#include <fstream>

#include <QJsonArray>

namespace imas{
namespace file{

class BXR
{
public:
    struct Offsetable
    {
        int32_t offset_symbol;
        std::string symbol;
    };

    template<class T>
    struct OffsetData : Offsetable
	{
        std::vector<T*> children;
	};

    struct MainScriptEntry;

    struct SubScriptEntry : Offsetable
    {
        int data_tag;	// tag_subScriptへ
        int next;
        std::string_view sub_symbol;
        std::optional<MainScriptEntry*> parent;
    };

    struct MainScriptEntry : Offsetable
    {
        int before;
        int data_tag;
        int index_sub_script;
        int next;
        int next_ticks;
        int offset_unicode;		// 歌詞

        std::string_view main_symbol;		// dataTag
        std::u16string unicode;			// 歌詞

        std::vector<MainScriptEntry*> children;
        std::vector<SubScriptEntry*> subscript_children;
    };

    bool load( std::string filename );
    void save(std::string const& filename);

    QJsonArray getJson();
    std::pair<bool, std::string> setJson(const QJsonValue &json);

    void writeXML(const std::string& filename);
private:
    std::vector<OffsetData<MainScriptEntry>> tag_main; // ダンス内の場合はroot/beat/data/start/words/camera/faces
    std::vector<OffsetData<SubScriptEntry>> tag_sub;  // ダンス内の場合はparam*/main_vocal_part/start_dance/start_title/wait/font/position_start/scale_x/rate* 等
    std::vector<MainScriptEntry> main_script;
    std::vector<SubScriptEntry> sub_script;
    MainScriptEntry* root;
    std::vector<std::reference_wrapper<MainScriptEntry>> unicode_containing_entries;

    uint32_t calculateSize();
    uint32_t calculateStringSize();
};

}   // namespace file
}	// namespace imas
