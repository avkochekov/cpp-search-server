#include "request_queue.h"

//=================================================================================
RequestQueue::RequestQueue(const SearchServer &search_server) : server(search_server) {}

//=================================================================================
std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query, DocumentStatus status) {
    return AddFindRequest(raw_query, [status]([[maybe_unused]] int document_id, DocumentStatus document_status, [[maybe_unused]]int rating) { return document_status == status; });
}

//=================================================================================
std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query) {
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

//=================================================================================
int RequestQueue::GetNoResultRequests() const {
    return no_resul_requests_count;
}
