#pragma once

//=================================================================================
#include "search_server.h"

//=================================================================================
#include <deque>

//=================================================================================
class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        int time = 0;
        bool is_empty = false;
    };

    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;

    int current_time_sec = 0;
    const SearchServer& server;
    int no_resul_requests_count = 0;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    auto result = server.FindTopDocuments(raw_query, document_predicate);

    requests_.push_back({++current_time_sec, result.empty()});
    no_resul_requests_count += result.empty();
    while (std::abs(requests_.front().time - current_time_sec) >= min_in_day_){
        no_resul_requests_count -= requests_.front().is_empty;
        requests_.pop_front();
    }
    return result;
}
