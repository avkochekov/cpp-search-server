#pragma once

//=================================================================================
#include <string>
#include <vector>
#include <set>

//=================================================================================
std::vector<std::string_view> SplitIntoWords(const std::string_view text);

using TransparentStringSet = std::set<std::string, std::less<>>;

template <typename StringContainer>
TransparentStringSet MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    TransparentStringSet non_empty_strings;
    for (const std::string_view str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(std::string(str));
        }
    }
    return non_empty_strings;
}
