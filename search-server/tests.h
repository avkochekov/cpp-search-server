#ifndef TESTS_H
#define TESTS_H

//=================================================================================
// Описание сервера
#include "searchserver.h"
// Hand-made фреймворк для юнит-тестов
#include "asserts.h"

//=================================================================================
//Добавление документов. Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа.
void TestAddDocumentAndFindAddedDocument() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    SearchServer server = SearchServer(string());
    {
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT(found_docs.empty());
    }
    {
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs.size(), 1U);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
}

//=================================================================================
//Поддержка стоп-слов. Стоп-слова исключаются из текста документов.
void TestExcludeStopWordsFromDocument() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    SearchServer server = SearchServer(string());
    server.SetStopWords("in the"s);
    {
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT(found_docs.empty());
    }
    {
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs.size(), 1U);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
}

//=================================================================================
//Поддержка минус-слов. Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска.
void TestMinusWordsSupport() {
    const vector<int> doc_ids = {42, 32, 33};
    const vector<string> contents = {"cat in the city"s, "dog in the city"s, "the cat and the mouse"s};
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server = SearchServer(string());
        server.SetStopWords("in the"s);
        for (size_t i = 0; i < doc_ids.size(); ++i){
            server.AddDocument(doc_ids[i], contents[i], DocumentStatus::ACTUAL, ratings);
        }
        {
            const auto found_docs = server.FindTopDocuments(""s);
            ASSERT(found_docs.empty());
        }
        {
            const auto found_docs = server.FindTopDocuments("cat"s);
            ASSERT_EQUAL(found_docs.size(), 2U);
            ASSERT_EQUAL(found_docs[0].id, doc_ids[0]);
            ASSERT_EQUAL(found_docs[1].id, doc_ids[2]);
        }
        {
            const auto found_docs = server.FindTopDocuments("cat -cat"s);
            ASSERT(found_docs.empty());
        }
        {
            const auto found_docs = server.FindTopDocuments("cat -mouse"s);
            ASSERT_EQUAL(found_docs.size(), 1U);
            const Document& doc0 = found_docs[0];
            ASSERT_EQUAL(doc0.id, doc_ids[0]);
        }
        {
            const auto found_docs = server.FindTopDocuments("-city"s);
            ASSERT(found_docs.empty());
        }
    }
}

//=================================================================================
//Матчинг документов. При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса, присутствующие в документе.
//Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
void TestMatchDocument() {
    const vector<int> doc_ids = {42, 32, 33};
    const vector<string> contents = {"cat in the city"s, "dog in the city"s, "the cat and the mouse"s};
    const vector<int> ratings = {1, 2, 3};

    SearchServer server = SearchServer(string());
    server.SetStopWords("in the"s);


    server.AddDocument(42, "cat in the city"s,          DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(43, "dog in the city"s,          DocumentStatus::ACTUAL, {3, 4, 5});
    server.AddDocument(32, "the cat and the mouse"s,    DocumentStatus::ACTUAL, {5, 6, 7});

    {
        const auto& [words, status] = server.MatchDocument("cat dog city"s, 42);
        ASSERT_EQUAL(words.size(), 2U);
        ASSERT_EQUAL(words, vector<string>({"cat"s, "city"s}));
    }
    {
        const auto& [words, status] = server.MatchDocument("cat dog -mouse"s, 43);
        ASSERT_EQUAL(words.size(), 1U);
        ASSERT_EQUAL(words, vector<string>({"dog"s}));
    }
    {
        const auto& [words, status] = server.MatchDocument("cat dog -city"s, 32);
        ASSERT_EQUAL(words.size(), 1U);
        ASSERT_EQUAL(words, vector<string>({"cat"s}));
    }
    {
        const auto& [words, status] = server.MatchDocument("cat -cat"s, 32);
        ASSERT(words.empty());
    }
}

//=================================================================================
//Сортировка найденных документов по релевантности. Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void TestSortSocumentsByRelevance() {
    SearchServer server = SearchServer(string());
    server.SetStopWords("in the"s);
    server.AddDocument(42, "cat in the city"s,          DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(43, "dog in the city"s,          DocumentStatus::ACTUAL, {3, 4, 5});
    server.AddDocument(32, "the cat and the mouse"s,    DocumentStatus::ACTUAL, {5, 6, 7});
    {
        const auto found_docs = server.FindTopDocuments("cat dog"s);
        ASSERT_EQUAL(found_docs.size(), 3U);
        ASSERT_EQUAL(found_docs[0].id, 43);
        ASSERT_EQUAL(found_docs[1].id, 42);
        ASSERT_EQUAL(found_docs[2].id, 32);
    }
    {
        const auto found_docs = server.FindTopDocuments("cat dog -mouse"s);
        ASSERT_EQUAL(found_docs.size(), 2U);
        ASSERT_EQUAL(found_docs[0].id, 43);
        ASSERT_EQUAL(found_docs[1].id, 42);
    }
    {
        const auto found_docs = server.FindTopDocuments("cat dog -city"s);
        ASSERT_EQUAL(found_docs.size(), 1U);
        ASSERT_EQUAL(found_docs[0].id, 32);
    }
    {
        const auto found_docs = server.FindTopDocuments("cat -cat"s);
        ASSERT(found_docs.empty());
    }
}

//=================================================================================
//Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void TestDocumentRatingCalculation(){
    const vector<int> doc_ids = {42, 32, 33};
    const vector<string> contents = {"cat in the city"s, "dog in the city"s, "the cat and the mouse"s};
    const vector<int> ratings = {2, 4, 6, -2, 0};

    SearchServer server = SearchServer(string());
    server.SetStopWords("in the"s);

    server.AddDocument(42, "cat in the city"s,          DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(43, "dog in the city"s,          DocumentStatus::ACTUAL, {3, 4, 5});
    server.AddDocument(32, "the cat and the mouse"s,    DocumentStatus::ACTUAL, {5, 6, 7});
    server.AddDocument(33, "mouse in the house"s,       DocumentStatus::ACTUAL, {-1, -2, -3});
    server.AddDocument(55, "cat and dog in the house"s, DocumentStatus::ACTUAL, {5, 0, -5});

    {
        const auto found_docs = server.FindTopDocuments("cat dog mouse"s);
        ASSERT_EQUAL(found_docs.size(), 5U);
        ASSERT_EQUAL(found_docs[0].rating, ratings[2]);
        ASSERT_EQUAL(found_docs[1].rating, ratings[1]);
        ASSERT_EQUAL(found_docs[2].rating, ratings[3]);
        ASSERT_EQUAL(found_docs[3].rating, ratings[4]);
        ASSERT_EQUAL(found_docs[4].rating, ratings[0]);
    }
}

//=================================================================================
//Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestDocumentFiltrationWithPredicate(){
    SearchServer server = SearchServer(string());
    server.SetStopWords("and in the"s);

    server.AddDocument(42, "cat in the city"s,          DocumentStatus::ACTUAL,     {1, 2, 3});
    server.AddDocument(43, "dog in the city"s,          DocumentStatus::IRRELEVANT, {3, 4, 5});
    server.AddDocument(32, "the cat and the mouse"s,    DocumentStatus::BANNED,     {5, 6, 7});
    server.AddDocument(33, "mouse in the house"s,       DocumentStatus::REMOVED,    {-1, -2, -3});
    server.AddDocument(55, "cat and dog in the house"s, DocumentStatus::IRRELEVANT, {5, 0, -5});

    {
        const auto found_docs = server.FindTopDocuments("cat in the city"s, []([[maybe_unused]] const int id, const DocumentStatus status, [[maybe_unused]] const int rating){
            return status == DocumentStatus::IRRELEVANT;
        });
        ASSERT_EQUAL(found_docs.size() ,2U);
        ASSERT_EQUAL(found_docs[0].id, 43);
        ASSERT_EQUAL(found_docs[1].id, 55);
    }
    {
        const auto found_docs = server.FindTopDocuments("cat in the city"s, []([[maybe_unused]] const int id, const DocumentStatus status, [[maybe_unused]] const int rating){
            return status == DocumentStatus::REMOVED;
        });
        ASSERT(found_docs.empty());
    }
    {
        const auto found_docs = server.FindTopDocuments("cat in the city"s, []([[maybe_unused]] const int id, const DocumentStatus status, const int rating){
            return status == DocumentStatus::IRRELEVANT && rating == 0;
        });
        ASSERT_EQUAL(found_docs.size(), 1U);
        ASSERT_EQUAL(found_docs[0].id, 55);
    }
    {
        const auto found_docs = server.FindTopDocuments("cat in the city"s, []([[maybe_unused]] const int id, const DocumentStatus status, const int rating){
            return status == DocumentStatus::IRRELEVANT && rating > 100;
        });
        ASSERT(found_docs.empty());
    }
    {
        const auto found_docs = server.FindTopDocuments("cat in the city"s, []([[maybe_unused]] const int id, const DocumentStatus status, [[maybe_unused]] const int rating){
            return status == DocumentStatus::IRRELEVANT || status == DocumentStatus::ACTUAL;
        });
        ASSERT_EQUAL(found_docs.size(), 3U);
        ASSERT_EQUAL(found_docs[0].id, 42);
        ASSERT_EQUAL(found_docs[1].id, 43);
        ASSERT_EQUAL(found_docs[2].id, 55);
    }
}

//=================================================================================
//Поиск документов, имеющих заданный статус.
void TestDocumentWithGivenStatus(){
    SearchServer server = SearchServer(string());
    server.SetStopWords("and in the"s);

    {
        const auto found_docs = server.FindTopDocuments("cat in the city"s, DocumentStatus::ACTUAL);
        ASSERT(found_docs.empty());
    }

    server.AddDocument(42, "cat in the city"s,          DocumentStatus::ACTUAL,     {1, 2, 3});
    server.AddDocument(43, "dog in the city"s,          DocumentStatus::IRRELEVANT, {3, 4, 5});
    server.AddDocument(32, "the cat and the mouse"s,    DocumentStatus::BANNED,     {5, 6, 7});
    server.AddDocument(33, "mouse in the house"s,       DocumentStatus::REMOVED,    {-1, -2, -3});
    server.AddDocument(55, "cat and dog in the house"s, DocumentStatus::IRRELEVANT, {5, 0, -5});

    {
        const auto found_docs = server.FindTopDocuments("cat in the city"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs.size(), 1U);
        ASSERT_EQUAL(found_docs[0].id, 42);
    }
    {
        const auto found_docs = server.FindTopDocuments("cat in the city"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(found_docs.size(), 2U);
        ASSERT_EQUAL(found_docs[0].id, 43);
        ASSERT_EQUAL(found_docs[1].id, 55);
    }
    {
        const auto found_docs = server.FindTopDocuments("cat in the city"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(found_docs.size(), 1U);
        ASSERT_EQUAL(found_docs[0].id, 32);
    }
    {
        const auto found_docs = server.FindTopDocuments("cat in the city"s, DocumentStatus::REMOVED);
        ASSERT(found_docs.empty());
    }
}

//=================================================================================
//Корректное вычисление релевантности найденных документов.
void TestCorectDocumentRelevanceCalculation() {

    SearchServer server = SearchServer(string());
    server.SetStopWords("and in the"s);

    server.AddDocument(42, "cat in the city"s,          DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(43, "dog in the city"s,          DocumentStatus::ACTUAL, {3, 4, 5});
    server.AddDocument(32, "the cat and the mouse"s,    DocumentStatus::ACTUAL, {5, 6, 7});
    server.AddDocument(33, "mouse in the house"s,       DocumentStatus::ACTUAL, {-1, -2, -3});
    server.AddDocument(55, "cat and dog in the house"s, DocumentStatus::ACTUAL, {5, 0, -5});

    const auto found_docs = server.FindTopDocuments("cat dog -mouse"s);
    ASSERT_EQUAL(found_docs.size(), 3U);

    const float d_relevance = 1E-5;
    ASSERT(abs(found_docs[0].relevance - 0.47570) < d_relevance);
    ASSERT(abs(found_docs[1].relevance - 0.45814) < d_relevance);
    ASSERT(abs(found_docs[2].relevance - 0.25541) < d_relevance);

    is_sorted(found_docs.begin(), found_docs.end(), [](const Document& lhv, const Document& rhv){
        return lhv.relevance >= rhv.relevance;
    });
}

//=================================================================================
// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server = SearchServer(string());
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1U);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server = SearchServer(string());
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
    {
        SearchServer server = SearchServer(string());
        server.SetStopWords(content);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments(content).empty());
    }
}

//=================================================================================
// Тест проверяет количество документов на сервере
void TestDocumentsCount(){
    SearchServer server = SearchServer(string());
    ASSERT_EQUAL(server.GetDocumentCount(), 0);
    server.AddDocument(1, "work smart no hard"s, DocumentStatus::ACTUAL, {100,100,100});
    server.AddDocument(2, "work smart no hard"s, DocumentStatus::ACTUAL, {101,101,101});
    server.AddDocument(3, "work smart no hard"s, DocumentStatus::ACTUAL, {102,102,102});
    ASSERT_EQUAL(server.GetDocumentCount(), 3);
}

#endif // TESTS_H
