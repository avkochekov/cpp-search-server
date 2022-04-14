#include "other.h"

#include <cmath>
#include <iostream>
#include <array>
#include <string_view>
#include <deque>

#include "log_duration.h"
#include "search_server.h"
#include "string_processing.h"

SearchServer::SearchServer(const std::string& words)
{
    SetStopWords(words);
}

void SearchServer::SetStopWords(const std::string_view words)
{
    using namespace std::string_literals;

    const auto str_vector = SplitIntoWords(words);

    for(const auto& word : str_vector)
    {
        if(!IsValidWord(std::string(word)))
        {
            throw std::invalid_argument("word {"s + std::string(word) + "} contains illegal characters"s);
        }

        stop_words_.insert(std::string(word));
    }
}

std::string_view SearchServer::AddUniqueWord(const std::string& word)
{
    return std::string_view(*(unique_words.insert(word).first));
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings)
{
    CheckIsValidDocument(document_id);

    const std::string doc = std::string(document);

    const auto words = SplitIntoWordsNoStop(doc);

    const double inv_word_count = 1.0 / words.size();

    std::unordered_set<std::string_view> document_words_ids;

    for (const auto word : words)
    {
        std::string_view current_word = AddUniqueWord(std::string(word));

        word_to_document_freqs_[current_word][document_id] += inv_word_count;
        document_words_ids.insert(current_word);
    }

    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, document_words_ids });
    document_ids_.emplace(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus doc_status) const
{
    return FindTopDocuments(raw_query, [doc_status](int document_id, DocumentStatus status, int rating) { return status == doc_status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const
{
    return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
}

int SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const
{
    return MatchDocument(std::execution::seq, raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy policy, const std::string_view raw_query, int document_id) const
{
    const Query query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;

    for(const auto w : documents_.at(document_id).words)
    {
        if(std::find(query.minus_words.begin(), query.minus_words.end(), w) != query.minus_words.end())
        {
            return {{}, documents_.at(document_id).status};
        }
    }

    std::copy_if(documents_.at(document_id).words.begin(), documents_.at(document_id).words.end(), std::back_inserter(matched_words), [&](const auto word)
    {
        return std::find(query.plus_words.begin(), query.plus_words.end(), word) != query.plus_words.end();
    });

  std::sort(matched_words.begin(), matched_words.end());
  matched_words.erase(std::unique(matched_words.begin(), matched_words.end()), matched_words.end());

    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy policy, const std::string_view raw_query, int document_id) const
{
    //LOG_DURATION_NS("MatchDocument");

    Query query;
    {
        //LOG_DURATION_NS("ParseQuery");
        query = ParseQuery(raw_query);
    }

    //LOG_DURATION_NS("MatchDocument");

    std::vector<std::string_view> matched_words(documents_.at(document_id).words.size());

    for(const auto w : documents_.at(document_id).words)
    {
        if(std::find(query.minus_words.begin(), query.minus_words.end(), w) != query.minus_words.end())
        {
            return {{}, documents_.at(document_id).status};
        }
    }

    std::transform(std::execution::par, documents_.at(document_id).words.begin(), documents_.at(document_id).words.end(), matched_words.begin(), [&](auto word)
    {
        return (std::find(query.plus_words.begin(), query.plus_words.end(), word) != query.plus_words.end() ? word : std::string_view(""));
    });

    std::sort(matched_words.begin(), matched_words.end());
  matched_words.erase(std::unique(matched_words.begin(), matched_words.end()), matched_words.end());

    if(!matched_words.empty() && matched_words[0] == "")
    {
        matched_words.erase(matched_words.begin());
    }

    return {matched_words, documents_.at(document_id).status};
}

std::set<int>::iterator SearchServer::begin() const
{
    return document_ids_.begin();
}

std::set<int>::iterator SearchServer::end() const
{
    return document_ids_.end();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    static std::map<std::string_view, double> result;

    for(auto& [str, freq_map] : word_to_document_freqs_)
    {
        auto it = freq_map.find(document_id);

        if(it != freq_map.end())
        {
            result[str] = it->second;
        }
    }

    return result;
}

void SearchServer::RemoveDocument(int document_id)
{
  RemoveDocument(std::execution::seq, document_id);
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy policy, int document_id)
{
    std::vector<std::string_view> words_ids(documents_[document_id].words.begin(), documents_[document_id].words.end());

    std::for_each(policy, words_ids.begin(), words_ids.end(), [document_id, this](std::string_view word)
    {
        word_to_document_freqs_.at(word).erase(document_id);
    });

    document_ids_.erase(document_id);
    documents_.erase(document_id);
}

void SearchServer::RemoveDocument(std::execution::parallel_policy policy, int document_id)
{
    std::vector<std::string_view> words_ids(documents_[document_id].words.begin(), documents_[document_id].words.end());

    std::for_each(policy, words_ids.begin(), words_ids.end(), [document_id, this](std::string_view word)
    {
        word_to_document_freqs_[word].erase(document_id);
    });

    document_ids_.erase(document_id);
    documents_.erase(document_id);
}

std::set<int> SearchServer::GetDuplicatedIds() const
{
    std::set<int> result;
    std::set<std::set<std::string_view>> unique_ids;

    auto current = std::begin(documents_);
    const auto last = std::end(documents_);

    while(current != last)
    {
        auto&& [key, value] = *current;

        if(unique_ids.count(std::set<std::string_view>(value.words.begin(), value.words.end())) == 0)
        {
            unique_ids.emplace(value.words.begin(), value.words.end());
        }
        else
        {
            result.emplace(key);
        }
        current++;
    }

    return result;
}

bool SearchServer::IsStopWord(const std::string& word) const
{
    return stop_words_.count(word);
}

std::deque<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const
{
    using namespace std::string_literals;

    const auto in_words = SplitIntoWords(text);

    std::deque<std::string_view> words(in_words.size());

    for (const auto word : in_words)
    {
        if(!IsValidWord(std::string(word)))
        {
            throw std::invalid_argument("word {"s + std::string(word) + "} contains illegal characters"s);
        }
    }

    std::transform(in_words.begin(), in_words.end(), words.begin(), [this](auto word)
    {
        return !IsStopWord(std::string(word)) ? word : std::string_view("");
    });

    auto it = std::remove_if(words.begin(), words.end(), [](auto word)
    {
        return word.empty();
    });
    words.erase(it, words.end());

    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings)
{
    if (ratings.empty())
    {
        return 0;
    }

    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const
{
    using namespace std::string_literals;

    if(!IsValidWord(std::string(text)))
    {
        throw std::invalid_argument("word contains illegal characters"s);
    }

    if (text[0] == '-')
    {
        return {text.substr(1), true, IsStopWord(text.data())};
    }

    return {text, false, IsStopWord(std::string(text))};
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const
{
    std::deque<std::string_view> words = SplitIntoWordsNoStop(text);

    std::deque<std::string_view> plus_words = words;
    std::deque<std::string_view> minus_words = words;

    plus_words.erase(std::remove_if(plus_words.begin(), plus_words.end(), [](auto word) { return word[0] == '-';	}), plus_words.end());

    minus_words.erase(std::remove_if(minus_words.begin(), minus_words.end(), [](auto word)	{ return word[0] != '-'; }), minus_words.end());

    std::transform(minus_words.begin(), minus_words.end(), minus_words.begin(), [](auto word) {	return word.substr(1); });

    return {plus_words, minus_words};
}

bool SearchServer::IsValidWord(const std::string& word)
{
    bool is_valid_char = std::none_of(word.begin(), word.end(), [](char c)
    {
        return c >= '\0' && c < ' ';
    });

    bool is_not_single_minus = !(word.size() == 1 && word[0] == '-');
    bool id_no_double_minus = !(word.size() >= 2 && word.substr(0, 2) == "--");

    return is_valid_char && is_not_single_minus && id_no_double_minus;
}

void SearchServer::CheckIsValidDocument(int document_id) const
{
    if(document_id < 0)
    {
        throw std::invalid_argument("document id is less than zero");
    }

    if(documents_.count(document_id))
    {
        throw std::invalid_argument("duplicate document id { id = " + std::to_string(document_id) + " }");
    }
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const
{
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentStatus document_status) const
{
    return FindAllDocuments(query, [document_status](int, DocumentStatus status, int) { return status == document_status; });
}
