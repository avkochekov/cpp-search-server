#ifndef ASSERTS_H
#define ASSERTS_H

//=================================================================================
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

//=================================================================================
using namespace std;

//=================================================================================
template <class TestFunc>
void RunTestImpl(TestFunc func, const string& funcName) {
    func();
    cerr << funcName << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl(func, #func)

//=================================================================================
template<typename Term>
void Print(ostream& out, Term data){
    bool is_first = true;
    for (const auto& term : data){
        if (!is_first){
            out << ", "s;
        }
        out << term;
        is_first = false;
    }
}

//=================================================================================
template<typename Term>
ostream& operator<<(ostream& out, vector<Term> data){
    out << "[";
    Print(out, data);
    out << "]";
    return out;
}

//=================================================================================
template<typename Term>
ostream& operator<<(ostream& out, set<Term> data){
    out << "{";
    Print(out, data);
    out << "}";
    return out;
}

//=================================================================================
template<typename Term1, typename Term2>
ostream& operator<<(ostream& out, map<Term1, Term2> data){
    out << "{";
    bool is_first = true;
    for (const auto& [key, value]: data){
        if (!is_first){
            out << ", "s;
        }
        out << key << ": " << value;
        is_first = false;
    }
    out << "}";
    return out;
}

//=================================================================================
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

//=================================================================================
void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

//=================================================================================
#endif // ASSERTS_H
