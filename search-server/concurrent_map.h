#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <mutex>
#include <execution>
#include <future>
#include <execution>
#include <iterator>

#include "log_duration.h"
#include "test_framework.h"

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access {
        std::lock_guard<std::mutex> ref_to_guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count) : bucket_count(bucket_count), maps(bucket_count), mutexes(bucket_count){ };

    Access operator[](const Key& key){
        size_t index = static_cast<size_t>(key) % bucket_count;
        return {std::lock_guard<std::mutex>(mutexes[index]), maps[index][key]};
    };

    size_t erase(const Value& val){
        size_t cnt = 0;
        for (size_t i = 0; i < maps.size(); ++i)
        {
            std::lock_guard lg(mutexes[i]);
            cnt += maps[i].erase(val);
        }
        return cnt;
    }

    std::map<Key, Value> BuildOrdinaryMap(){
        std::map<Key, Value> res;
        for (size_t i = 0; i < maps.size(); ++i)
        {
            std::lock_guard lg(mutexes[i]);
            res.insert(maps[i].begin(), maps[i].end());
        }
        return res;
    };

    size_t size(){
        size_t cnt = 0;
        for (const auto& map : maps){
            cnt += map.size();
        }
        return cnt;
    }

    const std::map<Key, Value>* cbegin(){
        return maps.cbegin();
    }

    const std::map<Key, Value>* cend(){
        return maps.cend();
    }

    std::map<Key, Value>* begin(){
        return maps.begin();
    }

    std::map<Key, Value>* end(){
        return maps.end();
    }

private:
    size_t bucket_count;
    std::vector<std::map<Key, Value>> maps;
    std::vector<std::mutex> mutexes;
};
