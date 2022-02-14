#include <cmath>
#include <numeric>

//=================================================================================
#include "search_server.h"
#include "string_processing.h"

//=================================================================================
SearchServer::SearchServer(const std::string &stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {}

//=================================================================================
std::vector<Document> SearchServer::FindTopDocuments(const std::string &raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status]([[maybe_unused]] int document_id, DocumentStatus document_status, [[maybe_unused]]int rating) { return document_status == status; });
}

//=================================================================================
int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

//=================================================================================
int SearchServer::GetDocumentId(int index) const
{
    if (index < 0 || static_cast<size_t>(index) > documents_.size() - 1){
        throw std::out_of_range("Индекс документа больше количества документов на вервере");
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
std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string &raw_query, int document_id) const {
    const Query query = ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
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

//=================================================================================
void SearchServer::CheckWord(const std::string &word) const
{
    if (IsEmptyWord(word)){
        throw std::invalid_argument("Cлово не может быть пустым");
    } else if (IsWordStartWithMinus(word)){
        throw std::invalid_argument("Лишний знак \"-\" перед словом: " + word);
    } else if (IsContainSpecialSymbols(word)){
        throw std::invalid_argument("слово не может содержать спецсимволы: " + word);
    }
}

//=================================================================================
void SearchServer::CheckStopWords() const
{
    for (const auto& word : stop_words_){
        CheckWord(word);
    }
}

//=================================================================================
bool SearchServer::IsStopWord(const std::string &word) const {
    return stop_words_.count(word) > 0;
}

//=================================================================================
bool SearchServer::IsContainSpecialSymbols(const std::string &word) const
{
    return any_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

//=================================================================================
bool SearchServer::IsEmptyWord(const std::string &word) const
{
    return word.empty();
}

//=================================================================================
bool SearchServer::IsWordStartWithMinus(const std::string &word) const
{
    return word[0] == '-';
}

//=================================================================================
bool SearchServer::IsValidDocumentId(const int id) const
{
    return id >= 0;
}

//=================================================================================
bool SearchServer::IsUniqueDocumentId(const int id) const
{
    for (const auto& [doc_id, doc] : documents_){
        if (doc_id == id){
            return false;
        }
    }
    return true;
}

//=================================================================================
std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string &text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

//=================================================================================
int SearchServer::ComputeAverageRating(const std::vector<int> &ratings) {
    if (ratings.empty()) {
        return 0;
    }
    return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

//=================================================================================
double SearchServer::ComputeWordInverseDocumentFreq(const std::string &word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

//=================================================================================
SearchServer::Query SearchServer::ParseQuery(const std::string &text) const {
    Query query;
    for (const std::string& word : SplitIntoWords(text)) {
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
SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }

    CheckWord(text);

    return {text, is_minus, IsStopWord(text)};
}

//=================================================================================
void SearchServer::AddDocument(int document_id, const std::string &document, DocumentStatus status, const std::vector<int> &ratings) {
    if (!IsValidDocumentId(document_id)){
        throw std::invalid_argument("ID документа не может быть отрицательным: " + std::to_string(document_id));
    }
    if (!IsUniqueDocumentId(document_id)){
        throw std::invalid_argument("Существует документ с ID: " + std::to_string(document_id));
    }
    if (IsContainSpecialSymbols(document)){
        throw std::invalid_argument("Документ не должен содержать спецсимволы");
    }
    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id,
                       DocumentData{
                           ComputeAverageRating(ratings),
                           status
                       });
}

//=================================================================================
void SearchServer::SetStopWords(const std::string &text) {
    for (const std::string& word : SplitIntoWords(text)) {
        stop_words_.insert(word);
    }
}
