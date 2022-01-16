/*
 * Дорогой ревьюер!
 * После сдачи первого спринта я решил последовать совету предыдущего ревьюера и переместил
 * поисковый сервер, чудесный hand-made фреймворк для юнит-тестов и сами тесты в отдельные h-файлы.
 * Надеюсь, это не будет воспринято как ошибка во время код-ревью, но намерение сделать код читаемым.
 *
 * Торжественно клянусь, что замышляю шалость, и только шалость! =)
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
int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}
