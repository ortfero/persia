// This file is part of persia library
// Copyright 2022 Andrei Ilin <ortfero@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once


#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <system_error>
#include <unordered_map>
#include <vector>

#include <persia/mapped_file.hpp>



namespace persia {


    enum class storage_error {
        ok,
        file_size_is_too_small,
        invalid_file_signature,
        mismatch_file_size,
        mismatch_item_size,
        file_is_corrupted
    }; // storage_error


    class storage_error_category : public std::error_category {

        char const* name() const noexcept override {
            return "persia";
        }

        std::string message(int code) const noexcept override {
            switch(storage_error(code)) {
            case storage_error::ok:
                return "Ok";
            case storage_error::file_size_is_too_small:
                return "Storage file is too small";
            case storage_error::invalid_file_signature:
                return "Invalid storage file signature";
            case storage_error::mismatch_file_size:
                return "Mismatch file size";
            case storage_error::mismatch_item_size:
                return "Mismatch item size";
            default:
                return "Unknown";
            }
        }
    };


    inline storage_error_category const storage_error_category;


    inline std::error_code make_error_code(storage_error e) noexcept {
        return {int(e), storage_error_category};
    }
    
} // namespace persia


namespace std {

    template <> struct is_error_code_enum<persia::storage_error> : true_type {};

} // std


namespace persia {
    
    
    namespace detail {
        
        struct alignas(8) header {
            unsigned char signature[4];
            std::uint32_t item_size{0};
            std::uint32_t capacity{0};
            std::uint32_t size{0};
        }; // header
        
        
        enum class marker: std::uint32_t {
            empty = 0, occupied = 0xFEEDDA1A
        };
        
        template<typename T > struct alignas(8) record {
            enum marker marker{persia::detail::marker::empty};
            T data;
        }; // record
        
    } // namespace detail
    
    
    using storage_index = std::uint32_t;
    
    
    template<typename Key,
             typename Value,
             class Adapter = Value,
             class Indices = std::unordered_map<Key, storage_index>>
    class storage {
        
        Indices occupied_indices_;
        std::vector<storage_index> free_indices_;
        mapped_file mapped_file_;
        detail::header* header_{nullptr};
        detail::record<Value>* records_{nullptr};
        
        
        template<class I, class R, class D> class basic_iterator {
        friend class storage;
        private:
            I index_it_;
            R* records_;
            
            basic_iterator(I index_it, R* records) noexcept
                : index_it_{index_it}, records_{records} { }
        
        public:
            
            basic_iterator() = delete;
            basic_iterator(basic_iterator const&) noexcept = default;
            basic_iterator& operator = (basic_iterator const&) noexcept = default;
            
            
            bool operator == (basic_iterator const& other) const noexcept {
                return index_it_ == other.index_it_;
            }
            
            
            bool operator != (basic_iterator const& other) const noexcept {
                return index_it_ != other.index_it_;
            }
            
            
            D& operator * () const noexcept { return records_[index_it_->second].data; }
            D* operator -> () const noexcept { return &records_[index_it_->second].data; }
            
            basic_iterator& operator ++ () noexcept {
                ++index_it_;
                return *this;
            }
            
            basic_iterator operator ++ (int) noexcept {
                auto current = *this;
                ++index_it_;
                return current;
            }
        }; // basic_iterator
        
    public:
    
        using key_type = Key;
        using value_type = Value;
        using adapter_type = Adapter;
        using indices_type = Indices;
        using size_type = std::uint32_t;
        using const_iterator = basic_iterator<typename Indices::const_iterator,
                                              detail::record<Value> const,
                                              Value const>;
        using iterator = basic_iterator<typename Indices::iterator,
                                        detail::record<Value>,
                                        Value>;
                                        
        class expected;
        
        
        static expected create(std::filesystem::path const& path,
                               size_type initial_capacity);
        
        static expected open(std::filesystem::path const& path,
                             size_type initial_capacity);
        
        
        storage() = default;
        storage(storage const&) = delete;
        storage& operator = (storage const&) = delete;
        storage(storage&&) noexcept = default;
        storage& operator = (storage&&) noexcept = default;
        
        explicit operator bool () const noexcept {
            return !!mapped_file_;
        }
        
        
        size_type capacity() const noexcept {
            return size_type(free_indices_.capacity());
        }
        
        
        size_type size() const noexcept {
            return size_type(occupied_indices_.size());
        }
        
        
        bool empty() const noexcept {
            return occupied_indices_.empty();
        }
        
        
        bool fully_occupied() const noexcept {
            return free_indices_.empty();
        }
        
        
        const_iterator begin() const noexcept {
            return const_iterator{occupied_indices_.begin(), records_};
        }
        
        
        const_iterator end() const noexcept {
            return const_iterator{occupied_indices_.end(), records_};
        }
        
        
        iterator begin() noexcept {
            return iterator{occupied_indices_.begin(), records_};
        }
        
        
        iterator end() noexcept {
            return iterator{occupied_indices_.end(), records_};
        }
        
        
        bool insert(Value const& value) {
            if(free_indices_.empty())
                return false;
            auto const index = free_indices_.back();
            free_indices_.pop_back();
            auto const key = Adapter::key_of(value);
            auto emplaced = occupied_indices_.try_emplace(key, index);
            if(!emplaced.second)
                return false;
            auto* record = records_ + index;
            record->marker = detail::marker::occupied;
            record->data = value;
            return true;
        }
        
        
        bool insert_or_assign(Value const& value) {
            auto const key = Adapter::key_of(value);
            auto emplaced = occupied_indices_.try_emplace(key, 0u);
            if(emplaced.second) {
                if(free_indices_.empty()) {
                    occupied_indices_.erase(emplaced.first);
                    return false;
                }
                auto const index = free_indices_.back();
                free_indices_.pop_back();
                emplaced.first->second = index;
                auto* record = records_ + index;
                record->marker = detail::marker::occupied;
                record->data = value;
                return true;
            }
            auto const index = emplaced.first->second;
            auto* record = records_ + index;
            record->data = value;
            return true;
        }
        
        
        bool erase(Key const& key) {
            auto index_found = occupied_indices_.find(key);
            if(index_found == occupied_indices_.end())
                return false;
            auto const index = index_found->second;
            free_indices_.push_back(index);
            auto* record = records_ + index;
            record->marker = detail::marker::empty;
            occupied_indices_.erase(index_found);
            return true;
        }
        
        
        Value const* find(Key const& key) const noexcept {
            auto const index_found = occupied_indices_.find(key);
            if(index_found == occupied_indices_.end())
                return nullptr;
            auto const index = index_found->second;
            return &records_[index].data;
        }
        
        
        Value* find(Key const& key) noexcept {
            auto const index_found = occupied_indices_.find(key);
            if(index_found == occupied_indices_.end())
                return nullptr;
            auto const index = index_found->second;
            return &records_[index].data;
        }
        
        
        void clear() noexcept {
            for(auto [key, index]: occupied_indices_) {
                records_[index].marker = detail::marker::empty;
                free_indices_.push_back(index);
            }
            occupied_indices_.clear();
        }
        
        
    private:
    
        storage(Indices&& occupied_indices,
                std::vector<storage_index>&& free_indices,
                mapped_file&& mapped_file,
                detail::header* header,
                detail::record<Value>* records) noexcept
            : occupied_indices_{std::move(occupied_indices)}
            , free_indices_{std::move(free_indices)}
            , mapped_file_{std::move(mapped_file)}
            , header_{header}
            , records_{records} {
        }
        
        
        static expected expand(std::filesystem::path const& path,
                               size_type initial_capacity);
    }; // storage
    
    template<typename K, typename V, class A, class I>
    class storage<K, V, A, I>::expected {
    private:
        std::error_code error_code_;
        storage storage_;
            
    public:
        
        expected(std::error_code ec)
            : error_code_{ec} {
        }
        
        
        expected(storage&& s)
            : storage_(std::move(s)) {
        }
        
        
        explicit operator bool () const noexcept {
            return !!storage_;
        }
        
        
        storage& operator * () & noexcept {
            return storage_;
        }
        
        
        storage&& operator * () && noexcept {
            return std::move(storage_);
        }
        
        
        storage* operator -> () noexcept {
            return &storage_;
        }
        
        
        std::error_code error() const noexcept {
            return error_code_;
        }
    }; // storage::expected
    
    
    template<typename K, typename V, class A, class I> typename storage<K, V, A, I>::expected
    storage<K, V, A, I>::create(std::filesystem::path const& path,
                                typename storage<K, V, A, I>::size_type initial_capacity) {
        if(initial_capacity == 0)
            return {make_error_code(storage_error::file_size_is_too_small)};
        auto* file = std::fopen(path.string().data(), "w+b");
        if(file == nullptr)
            return {std::error_code{int(errno), std::system_category()}};
        std::fclose(file);
        namespace fs = std::filesystem;
        auto const storage_size = sizeof(detail::header) + initial_capacity * sizeof(detail::record<V>);
        auto ec = std::error_code{};
        fs::resize_file(path, storage_size, ec);
        if(!!ec)
            return {ec};
        auto expected_file = mapped_file::create(path);
        if(!expected_file)
            return {expected_file.error()};
        auto* header = expected_file->cast<detail::header>(0);
        header->signature[0] = 0xDA;
        header->signature[1] = 0x1A;
        header->signature[2] = 0xF1;
        header->signature[3] = 0x1E;
        header->item_size = sizeof(V);
        header->capacity = initial_capacity;
        
        auto occupied_indices = I{};
        occupied_indices.reserve(initial_capacity);
        auto free_indices = std::vector<storage_index>{};
        free_indices.reserve(initial_capacity);
        for(auto i = 0u; i != initial_capacity; ++i)
            free_indices.push_back(i);
        auto* records = expected_file->cast<detail::record<V>>(sizeof(detail::header));
        for(auto* record = records; record != records + initial_capacity; ++record)
            new(record) detail::record<V>{};
        
        return {storage{std::move(occupied_indices),
                        std::move(free_indices),
                        std::move(*expected_file),
                        header,
                        records}};
    }
    
    
    template<typename K, typename V, class A, class I> typename storage<K, V, A, I>::expected
    storage<K, V, A, I>::open(std::filesystem::path const& path,
                              typename storage<K, V, A, I>::size_type initial_capacity) {
        auto expected_file = mapped_file::create(path);
        if(!expected_file)
            return {expected_file.error()};
        if(expected_file->size() < sizeof(detail::header) + sizeof(detail::record<V>))
            return {make_error_code(storage_error::file_size_is_too_small)};
        auto* header = expected_file->cast<detail::header>(0);
        if(header->signature[0] != 0xDA
            || header->signature[1] != 0x1A
            || header->signature[2] != 0xF1
            || header->signature[3] != 0x1E)
            return {make_error_code(storage_error::invalid_file_signature)};
        if(expected_file->size() != sizeof(detail::header) + header->capacity * sizeof(detail::record<V>))
            return {make_error_code(storage_error::mismatch_file_size)};
        if(sizeof(V) != header->item_size)
            return {make_error_code(storage_error::mismatch_item_size)};
        if(initial_capacity > header->capacity) {
            *expected_file = mapped_file{};
            return expand(path, initial_capacity);
        }
        auto occupied_indices = I{};
        occupied_indices.reserve(header->capacity);
        auto free_indices = std::vector<storage_index>{};
        free_indices.reserve(header->capacity);
        auto* records = expected_file->cast<detail::record<V>>(sizeof(detail::header));
        for(auto i = 0u; i != header->capacity; ++i) {
            auto* record = records + i;
            switch(record->marker) {
            case detail::marker::empty:
                free_indices.push_back(i);
                continue;
            case detail::marker::occupied:
                occupied_indices[A::key_of(record->data)] = i;
                continue;
            default:
                return {make_error_code(storage_error::file_is_corrupted)};
            }
        }
        return {storage{std::move(occupied_indices),
                        std::move(free_indices),
                        std::move(*expected_file),
                        header,
                        records}};

    }
    
    
    template<typename K, typename V, class A, class I> typename storage<K, V, A, I>::expected
    storage<K, V, A, I>::expand(std::filesystem::path const& path,
                                typename storage<K, V, A, I>::size_type initial_capacity) {
        namespace fs = std::filesystem;
        auto ec = std::error_code{};
        fs::resize_file(path, sizeof(detail::header) + initial_capacity * sizeof(detail::record<V>), ec);
        if(!!ec)
            return {ec};
        auto expected_file = mapped_file::create(path);
        if(!expected_file)
            return {expected_file.error()};
        auto occupied_indices = I{};
        occupied_indices.reserve(initial_capacity);
        auto free_indices = std::vector<storage_index>{};
        free_indices.reserve(initial_capacity);
        auto* header  = expected_file->cast<detail::header>(0);
        auto* records = expected_file->cast<detail::record<V>>(sizeof(detail::header));
        for(auto i = 0u; i != header->capacity; ++i) {
            auto* record = records + i;
            switch(record->marker) {
            case detail::marker::empty:
                free_indices.push_back(i);
                continue;
            case detail::marker::occupied:
                occupied_indices[A::key_of(record->data)] = i;
                continue;
            default:
                return {make_error_code(storage_error::file_is_corrupted)};
            }
        }
        for(auto i = header->capacity; i != initial_capacity; ++i) {
            new(records + i) detail::record<V>{};
            free_indices.push_back(i);
        }
        header->capacity = initial_capacity;
        return {storage{std::move(occupied_indices),
                        std::move(free_indices),
                        std::move(*expected_file),
                        header,
                        records}};
    }


} // namespace persia
