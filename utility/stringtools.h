#ifndef STRINGTOOLS_H
#define STRINGTOOLS_H

#include <codecvt>
#include <locale>

typedef std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> WidestringConv;

#endif // STRINGTOOLS_H
