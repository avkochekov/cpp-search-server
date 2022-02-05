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

//=================================================================================
using namespace std;

//=================================================================================
const int MAX_RESULT_DOCUMENT_COUNT = 5;

//=================================================================================
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

//=================================================================================
int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

//=================================================================================
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

//=================================================================================
struct Document {
//    Document() = default;
    Document(const int id = 0, const double relevance = 0, const int rating = 0)
        :id(id), relevance(relevance), rating(rating) {}
    int id;
    double relevance;
    int rating;
};

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

public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        for (const auto& word : stop_words_){
            if (IsEmptyWord(word)){
                throw invalid_argument("Стоп-слово не может быть пустым");
            }
            if (IsContainSpecialSymbols(word)){
                throw invalid_argument("Стоп-слово не может содержать спецсимволы");
            }
        }
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))
    {
    }

    void SetStopWords(const string& text);
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings);

    template<typename Predicate>
    vector<Document> FindTopDocuments(const string& raw_query, Predicate predicate) const{
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, predicate);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
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
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const;
    bool IsContainSpecialSymbols(const string& word) const;
    bool IsEmptyWord(const string& word) const;
    bool IsValidDocumentId(const int id) const;
    bool IsUniqueDocumentId(const int id) const;

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

    vector<string> SplitIntoWordsNoStop(const string& text) const;

    static int ComputeAverageRating(const vector<int>& ratings);

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const;

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const;

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
inline void SearchServer::SetStopWords(const string &text) {
    for (const string& word : SplitIntoWords(text)) {
        stop_words_.insert(word);
    }
}

//=================================================================================
inline void SearchServer::AddDocument(int document_id, const string &document, DocumentStatus status, const vector<int> &ratings) {
    if (!IsValidDocumentId(document_id)){
        throw invalid_argument("ID документа не может быть отрицательным: " + std::to_string(document_id));
    }
    if (!IsUniqueDocumentId(document_id)){
        throw invalid_argument("Существует документ с ID: " + std::to_string(document_id));
    }
    if (IsContainSpecialSymbols(document)){
        throw invalid_argument("Документ не должен содержать спецсимволы");
    }
    const vector<string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id,
                       DocumentData{
                           ComputeAverageRating(ratings),
                           status
                       });
}

//=================================================================================
inline vector<Document> SearchServer::FindTopDocuments(const string &raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status]([[maybe_unused]] int document_id, DocumentStatus document_status, [[maybe_unused]]int rating) { return document_status == status; });
}

//=================================================================================
inline int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

//=================================================================================
inline int SearchServer::GetDocumentId(int index) const
{
    if (index < 0 || static_cast<size_t>(index) > documents_.size() - 1){
        throw out_of_range("Индекс документа больше количества документов на вервере");
    }
    int doc_index = 0;
    int doc_id = INVALID_DOCUMENT_ID;
    for (const auto& [key, value] : documents_){
        if (doc_index == index){
            doc_id = key;
            break;
        }
        ++doc_index;
    }
    return doc_id;
}

//=================================================================================
inline bool SearchServer::IsStopWord(const string &word) const {
    return stop_words_.count(word) > 0;
}

//=================================================================================
inline bool SearchServer::IsContainSpecialSymbols(const string &word) const
{
    return any_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

//=================================================================================
inline bool SearchServer::IsEmptyWord(const string &word) const
{
    if (word.empty()){
        return true;
    } else if (word[0] == '-') {
        return true;
    } else {
        return false;
    }
}

//=================================================================================
inline bool SearchServer::IsValidDocumentId(const int id) const
{
    return id >= 0;
}

//=================================================================================
inline bool SearchServer::IsUniqueDocumentId(const int id) const
{
    for (const auto& [doc_id, doc] : documents_){
        if (doc_id == id){
            return false;
        }
    }
    return true;
}

//=================================================================================
inline vector<string> SearchServer::SplitIntoWordsNoStop(const string &text) const {
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

//=================================================================================
inline int SearchServer::ComputeAverageRating(const vector<int> &ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

//=================================================================================
inline double SearchServer::ComputeWordInverseDocumentFreq(const string &word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

//=================================================================================
inline SearchServer::Query SearchServer::ParseQuery(const string &text) const {
    Query query;
    for (const string& word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            } else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

//=================================================================================
inline SearchServer::QueryWord SearchServer::ParseQueryWord(string text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }

    if (IsContainSpecialSymbols(text)){
        throw invalid_argument("Запрос содержит сепцсимволы");
    }
    if (IsEmptyWord(text)){
        throw invalid_argument("Запрос пустой или имеет недопустимые символы");
    }

    return {text, is_minus, IsStopWord(text)
    };
}

//=================================================================================
#endif // SEARCHSERVER_H
