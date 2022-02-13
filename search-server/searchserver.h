#ifndef SEARCHSERVER_H
#define SEARCHSERVER_H

//=================================================================================
#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <numeric>

//=================================================================================
using namespace std;

//=================================================================================
const int MAX_RESULT_DOCUMENT_COUNT = 5;

//=================================================================================
string ReadLine();

//=================================================================================
int ReadLineWithNumber();

//=================================================================================
vector<string> SplitIntoWords(const string& text);

//=================================================================================
struct Document {
    Document(const int id = 0, const double relevance = 0, const int rating = 0);
    int id;
    double relevance;
    int rating;
};

//=================================================================================
inline ostream& operator<<(ostream& os, const Document& doc) {
    os << "{ document_id = " << doc.id << ", relevance = " << doc.relevance << ", rating = " << doc.rating << " }";
    return os;
}

//=================================================================================
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

//=================================================================================
class SearchServer {
    inline static constexpr int INVALID_DOCUMENT_ID = -1;
    inline static constexpr double DOUBLE_CALCULATION_ERROR = 1e-6;

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;


public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        CheckStopWords();
    }

    explicit SearchServer(const string& stop_words_text);

    void SetStopWords(const string& text);
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings);

    template<typename Predicate>
    vector<Document> FindTopDocuments(const string& raw_query, Predicate predicate) const{
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, predicate);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < DOUBLE_CALCULATION_ERROR) {
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

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const;

    int GetDocumentCount() const;
    int GetDocumentId(int index) const;

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:
    void CheckStopWords() const;
    bool IsStopWord(const string& word) const;
    bool IsContainSpecialSymbols(const string& word) const;
    bool IsEmptyWord(const string& word) const;
    bool IsWordStartWithMinus(const string& word) const;
    bool IsValidDocumentId(const int id) const;
    bool IsUniqueDocumentId(const int id) const;
    vector<string> SplitIntoWordsNoStop(const string& text) const;
    static int ComputeAverageRating(const vector<int>& ratings);
    double ComputeWordInverseDocumentFreq(const string& word) const;

    QueryWord ParseQueryWord(string text) const;

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const;

    template <typename StringContainer>
    set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
        set<string> non_empty_strings;
        for (const string& str : strings) {
            if (!str.empty()) {
                non_empty_strings.insert(str);
            }
        }
        return non_empty_strings;
    }

    template<typename Predicate>
    vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;

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

};

//=================================================================================
#endif // SEARCHSERVER_H
