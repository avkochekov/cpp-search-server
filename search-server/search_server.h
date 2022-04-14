#pragma once
//=================================================================================
#include <set>
#include <unordered_set>
#include <map>
#include <vector>
#include <algorithm>
#include <execution>
#include <list>

//=================================================================================
#include "document.h"
#include "string_processing.h"
#include "log_duration.h"

//=================================================================================
const int MAX_RESULT_DOCUMENT_COUNT = 5;

//=================================================================================
class SearchServer {
    inline static constexpr int INVALID_DOCUMENT_ID = -1;
    inline static constexpr double DOUBLE_CALCULATION_ERROR = 1e-6;

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::string text;
    };

    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;

    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    std::map<int, std::map<std::string, double>> document_to_word_freqs;

public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string& stop_words_text);

    void SetStopWords(const std::string& text);
    void AddDocument(int document_id, const std::string document, DocumentStatus status, const std::vector<int>& ratings);

    template<typename Predicate>
    std::vector<Document> FindTopDocuments(const std::string raw_query, Predicate predicate) const;
    std::vector<Document> FindTopDocuments(const std::string raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const;

    int GetDocumentCount() const;
    int GetDocumentId(int index) const;
    void RemoveDocument(int document_id);
    void RemoveDocument(std::execution::sequenced_policy& policy, int document_id);
    void RemoveDocument(std::execution::parallel_policy& policy, int document_id);

    const std::map<std::string, double>& GetWordFrequencies(int document_id) const;

    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string raw_query, int document_id) const;
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy& policy, const std::string raw_query, int document_id) const;
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::execution::parallel_policy& policy, const std::string raw_query, int document_id) const;

private:
    void CheckDocumentId(const int id) const;
    void CheckWord(const std::string& word) const;
    void CheckStopWords() const;
    bool IsStopWord(const std::string& word) const;
    bool IsContainSpecialSymbols(const std::string& word) const;
    bool IsEmptyWord(const std::string& word) const;
    bool IsWordStartWithMinus(const std::string& word) const;
    bool IsValidDocumentId(const int id) const;
    bool IsUniqueDocumentId(const int id) const;
    std::vector<std::string> SplitIntoWordsNoStop(const std::string text) const;
    static int ComputeAverageRating(const std::vector<int>& ratings);
    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    QueryWord ParseQueryWord(std::string text) const;

    struct Query {
        std::unordered_set<std::string> plus_words;
        std::unordered_set<std::string> minus_words;
    };

    Query ParseQuery(const std::string text) const;
    Query ParseQuery(const std::execution::sequenced_policy& policy, const std::string text) const;
    Query ParseQuery(const std::execution::parallel_policy& policy, const std::string text) const;

    template<typename Predicate>
    std::vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
    CheckStopWords();
}

template<typename Predicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string raw_query, Predicate predicate) const {
    const Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, predicate);

    std::sort(matched_documents.begin(), matched_documents.end(),
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
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, Predicate predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            document_to_relevance[document_id] += term_freq * inverse_document_freq;
        }
    }

    for (const std::string& word : query.minus_words) {
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
