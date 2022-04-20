#include <cmath>
#include <numeric>
#include <list>
#include <future>

#include <cassert>

//=================================================================================
#include "search_server.h"

//=================================================================================
SearchServer::SearchServer(const std::string_view stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {}

//=================================================================================
SearchServer::SearchServer(const std::string stop_words_text) : SearchServer(std::string_view(stop_words_text)) {}

//=================================================================================
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status]([[maybe_unused]] int document_id, DocumentStatus document_status, [[maybe_unused]]int rating) { return document_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(__pstl::execution::sequenced_policy, const std::string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(std::execution::seq, raw_query, [status]([[maybe_unused]] int document_id, DocumentStatus document_status, [[maybe_unused]]int rating) { return document_status == status; });
}

std::vector<Document> SearchServer::FindTopDocuments(__pstl::execution::parallel_policy, const std::string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(std::execution::par, raw_query, [status]([[maybe_unused]] int document_id, DocumentStatus document_status, [[maybe_unused]]int rating) { return document_status == status; });
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
void SearchServer::RemoveDocument(int document_id)
{
    for(auto &[word, mapa] : word_to_document_freqs_){
        mapa.erase(document_id);
    }
    document_to_word_freqs.erase(document_id);
    document_ids_.erase(document_id);
    documents_.erase(document_id);
}

//=================================================================================
void SearchServer::RemoveDocument([[maybe_unused]] __pstl::execution::sequenced_policy &policy, int document_id)
{
    RemoveDocument(document_id);
}

//=================================================================================
void SearchServer::RemoveDocument([[maybe_unused]] __pstl::execution::parallel_policy &policy, int document_id)
{
    if (!document_ids_.count(document_id))
        return;

    const auto& doc_words_freqs = document_to_word_freqs.at(document_id);

    std::vector<std::string_view> doc_words(doc_words_freqs.size());
    for (const auto &[word, freq] : doc_words_freqs){
        doc_words.push_back(word);
    }

    for_each(policy, doc_words.begin(),doc_words.end(),
             [&](const auto& word) {
            word_to_document_freqs_[word].erase(document_id); });

    document_to_word_freqs.erase(document_id);
    document_ids_.erase(document_id);
    documents_.erase(document_id);
}

//=================================================================================
const std::map<std::string_view, double> &SearchServer::GetWordFrequencies(int document_id) const
{
    if (document_to_word_freqs.count(document_id)){
        return document_to_word_freqs.at(document_id);
    } else {
        static std::map<std::string_view, double> empty;
        return empty;
    }
}

//=================================================================================
std::set<int>::const_iterator SearchServer::begin() const
{
    return document_ids_.cbegin();
}

//=================================================================================
std::set<int>::const_iterator SearchServer::end() const
{
    return document_ids_.cend();
}

//=================================================================================
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {
    return MatchDocument(std::execution::seq, raw_query, document_id);
}

//=================================================================================
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument([[maybe_unused]] const std::execution::sequenced_policy &policy, const std::string_view raw_query, int document_id) const
{
    CheckDocumentIdExistence(document_id);

    const Query &query = ParseQuery(raw_query);     // FIXED
    std::vector<std::string_view> matched_words;

    const auto &document_words = documents_.at(document_id).text;

    for (const std::string& word : document_words){
        auto iter = std::find(query.plus_words.begin(), query.plus_words.end(), word);
        if (iter != query.plus_words.end()){
            matched_words.push_back(word);
        }
    }

    for (const std::string& word : document_words){
        auto iter = std::find(query.minus_words.begin(), query.minus_words.end(), word);
        if (iter != query.minus_words.end()){
            matched_words.clear();
            break;
        }
    }

    std::sort(matched_words.begin(), matched_words.end());
    matched_words.erase(std::unique(matched_words.begin(), matched_words.end()), matched_words.end());

    return {matched_words, documents_.at(document_id).status};//
}

//=================================================================================
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument([[maybe_unused]] const std::execution::parallel_policy &policy, const std::string_view raw_query, int document_id) const
{
    CheckDocumentIdExistence(document_id);

    const Query &query = ParseQuery(raw_query);    // FIXED

    const auto check_word = [&document_id, this](const auto& word) {
        return document_to_word_freqs.at(document_id).count(word) > 0;
    };

    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(), check_word)) {
        return {std::vector<std::string_view>(), documents_.at(document_id).status};
    }

    std::vector<std::string_view> matched_words(query.plus_words.size());
    auto words_end = std::copy_if(std::execution::par,
                                  query.plus_words.begin(), query.plus_words.end(),
                                  matched_words.begin(),
                                  check_word);

    std::sort(matched_words.begin(), words_end);
    matched_words.erase(std::unique(matched_words.begin(), words_end), matched_words.end());

    return {matched_words, documents_.at(document_id).status};
}

//=================================================================================
void SearchServer::CheckDocumentIdExistence(const int id) const
{
    if (!document_ids_.count(id))
        throw std::invalid_argument("invalid document id");
}

//=================================================================================
void SearchServer::CheckWord(const std::string_view word) const
{
    if (IsEmptyWord(word)){
        throw std::invalid_argument("empty word");
    } else if (IsWordStartWithMinus(word)){
        throw std::invalid_argument("extra sign \'-\': " + std::string(word));
    } else if (IsContainSpecialSymbols(word)){
        throw std::invalid_argument("word contain special symbols: " + std::string(word));
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
bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}

//=================================================================================
bool SearchServer::IsContainSpecialSymbols(const std::string_view word) const
{
    return std::any_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

//=================================================================================
bool SearchServer::IsEmptyWord(const std::string_view word) const
{
    return word.empty();
}

//=================================================================================
bool SearchServer::IsWordStartWithMinus(const std::string_view word) const
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
    return document_ids_.count(id) == 0;
}

//=================================================================================
std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view > words;
    for (const std::string_view word : SplitIntoWords(text)) {
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
double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

//=================================================================================
SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const {
    Query query;

    for (const std::string_view word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            } else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }

    return query;
}

//=================================================================================
void SearchServer::SortQuery(Query &query) const
{
    for (auto* words : {&query.plus_words, &query.minus_words}){
        std::sort(words->begin(), words->end());
        words->erase(std::unique(words->begin(), words->end()), words->end());
    }
}

//=================================================================================
SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const{
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
void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int> &ratings) {
    if (!IsValidDocumentId(document_id)){
        throw std::invalid_argument("negative document id: " + std::to_string(document_id));
    }
    if (!IsUniqueDocumentId(document_id)){
        throw std::invalid_argument("document id is exist: " + std::to_string(document_id));
    }
    if (IsContainSpecialSymbols(document)){
        throw std::invalid_argument("document contaion special symbols");
    }

    const auto words = SplitIntoWordsNoStop(document);
    std::vector<std::string> document_words(words.size());
    std::transform(std::execution::par,
                   words.begin(), words.end(),
                   document_words.begin(),
                   [](std::string_view word){ return std::string(word);});

    document_ids_.insert(document_id);
    documents_.emplace(document_id,
                       DocumentData{
                           ComputeAverageRating(ratings),
                           status,
                           document_words
                           });

    const double inv_word_count = 1.0 / words.size();
    for (const std::string &word : documents_.at(document_id).text) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs[document_id][word] += inv_word_count;
    }
}

//=================================================================================
void SearchServer::SetStopWords(const std::string_view text) {
    for (const std::string_view word : SplitIntoWords(text)) {
        stop_words_.insert(std::string(word));
    }
}
