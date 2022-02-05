/*
 * Дорогой ревьюер!
 * После сдачи первого спринта я решил последовать совету ревьюера, проверявшего решение, и переместил
 * поисковый сервер, чудесный hand-made фреймворк для юнит-тестов и сами тесты в отдельные h-файлы.
 * Надеюсь, это не будет воспринято как ошибка во время код-ревью, но намерение сделать код читаемым.
 *
 * Торжественно клянусь, что замышляю шалость, и только шалость! =)
 *
 * UPD 05.02.2022 - СПРИНТ 3
 * Изменения, требуемые для сдаче 3 спринта, находятся в searchserver.h
 */

//=================================================================================
// Описание сервера
#include "searchserver.h"
// Юнит-тесты для сервера
#include "tests.h"

//=================================================================================
// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestAddDocumentAndFindAddedDocument);
    RUN_TEST(TestExcludeStopWordsFromDocument);
    RUN_TEST(TestMinusWordsSupport);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestSortSocumentsByRelevance);
    RUN_TEST(TestDocumentRatingCalculation);
    RUN_TEST(TestDocumentFiltrationWithPredicate);
    RUN_TEST(TestDocumentWithGivenStatus);
    RUN_TEST(TestCorectDocumentRelevanceCalculation);
    RUN_TEST(TestDocumentsCount);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
}

//=================================================================================
void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
    cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}

void AddDocument(SearchServer& search_server, int document_id, const string& document, DocumentStatus status,
                 const vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const exception& e) {
        cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const string& raw_query) {
    cout << "Результаты поиска по запросу: "s << raw_query << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const exception& e) {
        cout << "Ошибка поиска: "s << e.what() << endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const string& query) {
    try {
        cout << "Матчинг документов по запросу: "s << query << endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const exception& e) {
        cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
    }
}

//=================================================================================
int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;

    SearchServer search_server("и в на"s);

    AddDocument(search_server, 1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, -1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, 3, "большой пёс скво\x12рец евгений"s, DocumentStatus::ACTUAL, {1, 3, 2});
    AddDocument(search_server, 4, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, {1, 1, 1});

    FindTopDocuments(search_server, "пушистый -пёс"s);
    FindTopDocuments(search_server, "пушистый --кот"s);
    FindTopDocuments(search_server, "пушистый -"s);

    MatchDocuments(search_server, "пушистый пёс"s);
    MatchDocuments(search_server, "модный -кот"s);
    MatchDocuments(search_server, "модный --пёс"s);
    MatchDocuments(search_server, "пушистый - хвост"s);
}
