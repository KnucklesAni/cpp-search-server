#pragma once

#include <map>
#include <mutex>
#include <vector>

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    struct Data {
        std::mutex mut_;
        std::map<Key, Value> map_;
    };

    explicit ConcurrentMap(size_t bucket_count) : bucket_count_(bucket_count), data_(bucket_count_) { }

    Access operator[](const Key& key) {
        auto& ref = data_[static_cast<uint64_t>(key) % bucket_count_];
        return { std::lock_guard(ref.mut_), ref.map_[key] };
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& data_piece : data_) {
            std::lock_guard guard(data_piece.mut_);
            result.insert(data_piece.map_.begin(), data_piece.map_.end());
        }
        return result;
    }

    void Erase(const Key& key) {
        Data& ref = data_[static_cast<uint64_t>(key) % bucket_count_];
        std::lock_guard guard(ref.mut_);
        ref.map_.erase(key);
    }

private:
    size_t bucket_count_;
    std::vector<Data> data_;
};
