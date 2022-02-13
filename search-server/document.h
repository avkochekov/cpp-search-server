#pragma once

//=================================================================================
#include <iostream>

//=================================================================================
struct Document {
    Document(const int id = 0, const double relevance = 0, const int rating = 0);
    int id;
    double relevance;
    int rating;
};

//=================================================================================
std::ostream& operator<<(std::ostream& os, const Document& doc);

//=================================================================================
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};
