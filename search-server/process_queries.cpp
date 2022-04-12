#include "process_queries.h"

#include <execution>
#include <numeric>
#include <utility>

std::vector<std::vector<Document>> ProcessQueries(const SearchServer &search_server, const std::vector<std::string> &queries)
{
    std::vector<std::vector<Document>>documents_lists(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), documents_lists.begin(), [&search_server](const std::string& query){
        return search_server.FindTopDocuments(query);
    });
    return documents_lists;
}

std::list<Document> ProcessQueriesJoined(const SearchServer &search_server, const std::vector<std::string> &queries)
{
    std::list<Document> docs;
    for (auto &ds : ProcessQueries(search_server, queries)){
        for (auto &d : ds)
            docs.push_back(std::move(d));
    }
    return docs;
}
