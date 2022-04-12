#include "remove_duplicates.h"

//=================================================================================
void RemoveDuplicates(SearchServer &search_server)
{
    std::set<std::set<std::string>> uniq_words_set;
    std::set<int> for_removing;
    for (auto iter = search_server.begin(); iter != search_server.end(); ++iter){
        const auto& words = search_server.GetWordFrequencies(*iter);
        std::set<std::string> set;
        for (const auto& [word, freq] : words){
            set.insert(std::string(word));
        }

        if (uniq_words_set.count(set)){
            for_removing.insert(*iter);
        } else {
            uniq_words_set.insert(set);
        }
    }

    for (auto iter = for_removing.crbegin(); iter != for_removing.crend(); ++iter){
        std::cout << "Found duplicate document id " << *iter << std::endl;
        search_server.RemoveDocument(*iter);
    }
}
