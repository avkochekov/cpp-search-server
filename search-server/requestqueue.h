#ifndef REQUESTQUEUE_H
#define REQUESTQUEUE_H

#include <searchserver.h>
#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server) : server(search_server){}
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
        auto result = server.FindTopDocuments(raw_query, document_predicate);

        requests_.push_back({++current_time_sec, result.empty()});
        no_resul_requests_count += result.empty();
        cout << std::abs(requests_.front().time - current_time_sec) << ' ' << endl;
        while (std::abs(requests_.front().time - current_time_sec) >= min_in_day_){
            no_resul_requests_count -= requests_.front().is_empty;
            requests_.pop_front();
        }
        return result;
    }

    vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status) {
        return AddFindRequest(raw_query, [status]([[maybe_unused]] int document_id, DocumentStatus document_status, [[maybe_unused]]int rating) { return document_status == status; });
    }

    vector<Document> AddFindRequest(const string& raw_query) {
        return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
    }

    int GetNoResultRequests() const {
        return no_resul_requests_count;
    }
private:
    struct QueryResult {
        int time = 0;
        bool is_empty = false;
        // определите, что должно быть в структуре
    };
    deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;

    int current_time_sec = 0;
    const SearchServer& server;
    int no_resul_requests_count = 0;
    // возможно, здесь вам понадобится что-то ещё
};

#endif // REQUESTQUEUE_H
