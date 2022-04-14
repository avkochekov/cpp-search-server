#include "string_processing.h"

//=================================================================================
#include <set>
#include <algorithm>

//=================================================================================
std::vector<std::string_view> SplitIntoWords(const std::string_view text) {
    std::vector<std::string_view> words;

    auto s_begin = text.begin();
    for (auto s_end = s_begin; s_end != text.end();){
        s_end = std::find(s_begin, text.end(), ' ');
        if (s_end == s_begin){
            ++s_begin;
        } else {
            std::string_view word(s_begin, std::distance(s_begin, s_end));
            if (!word.empty()){
                words.push_back(word);
            }
        }
        s_begin = s_end + 1;
    }

    return words;
}
