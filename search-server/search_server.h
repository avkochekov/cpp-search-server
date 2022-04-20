#pragma once
//=================================================================================
#include <set>
#include <unordered_set>
#include <map>
#include <vector>
#include <algorithm>
#include <execution>
#include <list>
#include <thread>
#include <atomic>

//=================================================================================
#include "document.h"
#include "string_processing.h"
#include "log_duration.h"
#include "concurrent_map.h"

//=================================================================================
const int MAX_RESULT_DOCUMENT_COUNT = 5;

//=================================================================================
class SearchServer {
    inline static constexpr int INVALID_DOCUMENT_ID = -1;
    inline static constexpr double DOUBLE_CALCULATION_ERROR = 1e-6;

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::vector<std::string> text;
    };

    TransparentStringSet stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs;

    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string stop_words_text);
    explicit SearchServer(const std::string_view stop_words_text);

    void SetStopWords(const std::string_view text);
    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template<typename Predicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, Predicate predicate) const;
    template<typename Predicate>
    std::vector<Document> FindTopDocuments(std::execution::sequenced_policy, const std::string_view raw_query, Predicate predicate) const;
    template<typename Predicate>
    std::vector<Document> FindTopDocuments(std::execution::parallel_policy, const std::string_view raw_query, Predicate predicate) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const;
    std::vector<Document> FindTopDocuments(std::execution::parallel_policy, const std::string_view raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const;
    std::vector<Document> FindTopDocuments(std::execution::sequenced_policy, const std::string_view raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const;

    int GetDocumentCount() const;
    int GetDocumentId(int index) const;
    void RemoveDocument(int document_id);
    void RemoveDocument(std::execution::sequenced_policy& policy, int document_id);
    void RemoveDocument(std::execution::parallel_policy& policy, int document_id);

    const std::map<std::string_view, double> &GetWordFrequencies(int document_id) const;

    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy& policy, const std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy& policy, const std::string_view raw_query, int document_id) const;

private:
    void CheckDocumentIdExistence(const int id) const;
    void CheckWord(const std::string_view word) const;
    void CheckStopWords() const;
    bool IsStopWord(const std::string_view word) const;
    bool IsContainSpecialSymbols(const std::string_view word) const;
    bool IsEmptyWord(const std::string_view word) const;
    bool IsWordStartWithMinus(const std::string_view word) const;
    bool IsValidDocumentId(const int id) const;
    bool IsUniqueDocumentId(const int id) const;
    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;
    static int ComputeAverageRating(const std::vector<int>& ratings);
    double ComputeWordInverseDocumentFreq(std::string_view word) const;

    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(const std::string_view text) const;

    void SortQuery(Query& query) const;

    template<typename Predicate>
    std::vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const;
    template<typename Predicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, Predicate predicate) const;
    template<typename Predicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, const Query& query, Predicate predicate) const;
};

template <typename StringContainer>
inline SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
    CheckStopWords();
}

template<typename Predicate>
inline std::vector<Document> SearchServer::FindAllDocuments(const Query& query, Predicate predicate) const {
    return FindAllDocuments(std::execution::seq, query, predicate);
}

template<typename Predicate>
inline std::vector<Document> SearchServer::FindAllDocuments(const __pstl::execution::sequenced_policy &, const Query &query, Predicate predicate) const
{
    std::map<int, double> document_to_relevance;
    for (std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            document_to_relevance[document_id] += term_freq * inverse_document_freq;
        }
    }

    for (std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;

    for (const auto [document_id, relevance] : document_to_relevance) {
        if (predicate(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)){
            matched_documents.push_back({
                document_id,
                relevance,
                documents_.at(document_id).rating
            });
        }
    }
    return matched_documents;
}

template<typename Predicate>
inline std::vector<Document> SearchServer::FindAllDocuments(const __pstl::execution::parallel_policy &, const Query &query, Predicate predicate) const
{
    ConcurrentMap<int, double> document_to_relevance(document_ids_.size());

    for_each(std::execution::par,
             query.plus_words.begin(), query.plus_words.end(),
             [&document_to_relevance, this, &predicate, &query](std::string_view word)
    {
        auto contain_minus = std::any_of(std::execution::par,
                                         query.minus_words.begin(), query.minus_words.end(),
                                         [&query, &word](const auto& minus_word)
        {
            return minus_word == word;
        });

        if (word_to_document_freqs_.count(word) && !contain_minus) {
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            std::for_each(std::execution::par,
                          word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(),
                          [this, &document_to_relevance, &inverse_document_freq, &predicate, &query](const auto& doc_freq)
            {
                const auto& doc_data = documents_.at(doc_freq.first);
                if (predicate(doc_freq.first, doc_data.status, doc_data.rating)) {
                    document_to_relevance[doc_freq.first].ref_to_value += doc_freq.second * inverse_document_freq;
                }
            });
        }
    });

    const std::map<int, double>& ord_map = document_to_relevance.BuildOrdinaryMap();
    std::vector<Document> matched_documents(ord_map.size());
    std::atomic_int size = 0;
    std::for_each(std::execution::par,
                  ord_map.begin(), ord_map.end(),
                  [&matched_documents, this, &size](const auto& map)
    {
        matched_documents[size++] = {map.first, map.second, documents_.at(map.first).rating};
    });

    matched_documents.resize(size);

    return matched_documents;
}

template<typename Predicate>
inline std::vector<Document> SearchServer::FindTopDocuments(__pstl::execution::sequenced_policy, const std::string_view raw_query, Predicate predicate) const
{
    Query query = ParseQuery(raw_query);

    SortQuery(query);

    auto matched_documents = FindAllDocuments(std::execution::seq, query, predicate);

    std::sort(std::execution::seq,
              matched_documents.begin(), matched_documents.end(),
         [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < DOUBLE_CALCULATION_ERROR) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            }
         });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template<typename Predicate>
inline std::vector<Document> SearchServer::FindTopDocuments(__pstl::execution::parallel_policy, const std::string_view raw_query, Predicate predicate) const
{
    Query query = ParseQuery(raw_query);

    SortQuery(query);

    auto matched_documents = FindAllDocuments(std::execution::par, query, predicate);

    std::sort(std::execution::par,
              matched_documents.begin(), matched_documents.end(),
              [](const Document& lhs, const Document& rhs)
    {
        if (std::abs(lhs.relevance - rhs.relevance) < DOUBLE_CALCULATION_ERROR) {
            return lhs.rating > rhs.rating;
        } else {
            return lhs.relevance > rhs.relevance;
        }
    });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template<typename Predicate>
inline std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, Predicate predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, predicate);
}
