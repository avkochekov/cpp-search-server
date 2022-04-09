#include "remove_duplicates.h"

//=================================================================================
void RemoveDuplicates(SearchServer &search_server)
{
    std::set<std::set<std::string>> uniq_words_set;
    std::set<int> for_removing;
    for (auto iter = search_server.begin(); iter != search_server.end(); ++iter){
        const auto& words = search_server.GetWordFrequencies(*iter);
        std::set<std::string> uniq;

        for (const auto& word : words){
            uniq.insert(word.first);
        }

        if (uniq_words_set.count(uniq)){
            for_removing.insert(*iter);
        } else {
            uniq_words_set.insert(uniq);
        }

    }

    for (auto iter = for_removing.crbegin(); iter != for_removing.crend(); ++iter){
        std::cout << "Found duplicate document id " << *iter << std::endl;
        search_server.RemoveDocument(*iter);
    }
}
