#include "searchserver.h"

//=================================================================================
inline std::string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

//=================================================================================
inline int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

//=================================================================================
inline vector<string> SplitIntoWords(const string &text) {
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
Document::Document(const int id, const double relevance, const int rating)
    :id(id), relevance(relevance), rating(rating) {}

//=================================================================================
SearchServer::SearchServer(const string &stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text)) {}

//=================================================================================
vector<Document> SearchServer::FindTopDocuments(const string &raw_query, DocumentStatus status) const {
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
void SearchServer::CheckStopWords() const
{
    for (const auto& word : stop_words_){
        if (IsEmptyWord(word)){
            throw invalid_argument("Стоп-слово не может быть пустым");
        } else if (IsWordStartWithMinus(word)){
            throw invalid_argument("Лишний знак \"-\" перед стоп-словом: " + word);
        } else if (IsContainSpecialSymbols(word)){
            throw invalid_argument("Стоп-слово не может содержать спецсимволы: " + word);
        }
    }
}

//=================================================================================
bool SearchServer::IsStopWord(const string &word) const {
    return stop_words_.count(word) > 0;
}

//=================================================================================
bool SearchServer::IsContainSpecialSymbols(const string &word) const
{
    return any_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

//=================================================================================
bool SearchServer::IsEmptyWord(const string &word) const
{
    return word.empty();
}

//=================================================================================
bool SearchServer::IsWordStartWithMinus(const string &word) const
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
vector<string> SearchServer::SplitIntoWordsNoStop(const string &text) const {
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

//=================================================================================
int SearchServer::ComputeAverageRating(const vector<int> &ratings) {
    if (ratings.empty()) {
        return 0;
    }
    return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

//=================================================================================
double SearchServer::ComputeWordInverseDocumentFreq(const string &word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

//=================================================================================
SearchServer::Query SearchServer::ParseQuery(const string &text) const {
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
SearchServer::QueryWord SearchServer::ParseQueryWord(string text) const {
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
void SearchServer::AddDocument(int document_id, const string &document, DocumentStatus status, const vector<int> &ratings) {
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
void SearchServer::SetStopWords(const string &text) {
    for (const string& word : SplitIntoWords(text)) {
        stop_words_.insert(word);
    }
}
