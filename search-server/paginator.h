#pragma once

//=================================================================================
#include <iostream>
#include <vector>
#include <cassert>

//=================================================================================
template <typename Iterator>
class IteratorRange{
    Iterator range_begin;
    Iterator range_end;
    size_t range_size;
public:
    IteratorRange(Iterator begin, Iterator end) : range_begin {begin}, range_end{end} {
        range_size = std::distance(range_begin, range_end);
    }
    Iterator end() const {
        return range_end;
    }
    Iterator begin() const {
        return range_begin;
    }
    Iterator size() const {
        return range_size;
    }
};

//=================================================================================
template <typename Iterator>
class Paginator{
    std::vector<IteratorRange<Iterator>> range_;
public:
    Paginator(Iterator begin, Iterator end, const size_t page_size){
//        assert(("Размер страницы должен быть больше 0", page_size > 0));
        if (page_size == 0){
            throw std::invalid_argument("Размер страницы должен быть больше 0");
        }
//        assert(("Неверно указан диапазон данных", end >= begin));
        if (begin > end){
            throw std::invalid_argument("Неверно указан диапазон данных");
        }
        for (auto start_iter = begin; start_iter < end; start_iter += page_size) {
            auto next_iter = start_iter + page_size;
            if (next_iter > end)
                next_iter = end;
            range_.push_back({ start_iter, next_iter });
        }
    }

    auto begin() const {
        return range_.begin();
    }
    auto end() const {
        return range_.end();
    }
};

//=================================================================================
template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

//=================================================================================
template <typename Iterator>
std::ostream& operator<<(std::ostream& os, const IteratorRange<Iterator>& range) {
    for (const auto &it : range) {
        os << it;
    }
    return os;
}
