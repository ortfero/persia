// This file is part of persia library
// Copyright 2022 Andrei Ilin <ortfero@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once


#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <optional>
#include <system_error>
#include <unordered_map>

#if defined(_WIN32)

#if !defined(_X86_) && !defined(_AMD64_) && !defined(_ARM_) && !defined(_ARM64_)
#if defined(_M_IX86)
#define _X86_
#elif defined(_M_AMD64)
#define _AMD64_
#elif defined(_M_ARM)
#define _ARM_
#elif defined(_M_ARM64)
#define _ARM64_
#endif
#endif

#include <minwindef.h>
#include <memoryapi.h>
#include <handleapi.h>

#elif defined(__unix__) || defined(__MACH__)

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#else

#error Unsupported system

#endif


namespace persia {
    
    
    template<class E>
    class unexpected {
    private:
        E value_;
        
    public:
    
        unexpected() = delete;
        explicit constexpr unexpected(E const& e): value_{e} { }
        explicit constexpr unexpected(E&& e): value_{std::move(e)} { }
        constexpr E const& value() const noexcept { return value_; }
        constexpr E& value() noexcept { return value_; }
        constexpr E&& value() noexcept { return value_; }
        
    };
    
    
    template<class E> unexpected<std::decay_t<E>> make_unexpected(E&& e) {
        return unexpected<E>{std::forward<E>(e)};
    }
    
    
    template<typename T, typename E>
    class expected {
    public:
        
        using value_type = T;
        using error_type = E;
        using unexpected_type = unexpected<E>;
        
        constexpr expected(expected const&) = default;
        constexpr expected& operator = (expected const&) = default;
        constexpr expected(expected&&) = default;
        constexpr expected& operator = (expected&&) = default;
        
        explicit expected(T const& value): kind_{kind::of_value}, data_{value} { }
        explicit expected(T&& value): kind_{kind::of_value}, data_{std::move(value)} { }
        expected(unexpected<E> const& error): kind_{kind::of_error}, data_{error.value()} { }
        expected(unexpected<E>&& error): kind_{kind::of_error}, data_{std::move(error.value()} { }
        constexpr T* operator -> () noexcept { return &data_.value; }
        constexpr T const* operator -> () const noexcept { return &data_.value_; }
        constexpr T& operator * () & noexcept { return data_.value; }
        constexpr T const& operator * () const noexcept { return data_.value; }
        constexpr T&& operator * () && noexcept { return data_.value; }
        constexpr T& value() & noexcept { return data_.value; }
        constexpr T const& value() const noexcept { return data_.value; }
        constexpr T&& value() && noexcept { return data_.value; }
        constexpr E& error() noexcept { return data_.error; }
        constexpr E const& error() noexcept { return data_.error; }
        constexpr E&& error() noexcept { return data_.error; }
        constexpr bool has_value() const noexcept { return kind_ == kind::of_value; }
        explicit constexpr operator bool () const noexcept { return kind_ == kind::of_value; }
        
        constexpr T const& value_or(T const& value) const {
            switch(kind_) {
            case kind::of_value:
                return data_.value;
            case kind::of_error:
                return value;
            }
        }
        
        
        constexpr T value_or(T&& value) {
            switch(kind_) {
            case kind::of_value:
                return std::move(data_.value);
            case kind::of_error:
                return std::move(value);
            }
        }
        
        
        ~expected() {
            switch(kind_) {
            case kind::of_value:
                data_.value.~T();
                return;
            case kind::of_error:
                data_.error.~E();
            }
        }
        
        
    private:
        enum class kind { of_value, of_error } kind_
        union {
            T value;
            E error;
        } data_;
    }; // expected
    
    
    template<typename E>
    class expected<void, E> {
    public:
    
        using value_type = void;
        using error_type = E;
        using unexpected_type = unexpected<E>;
        
        constexpr expected(expected const&) = default;
        constexpr expected& operator = (expected const&) = default;
        constexpr expected(expected&&) = default;
        constexpr expected& operator = (expected&&) = default;
        
        explicit expected(): kind_{kind::of_value} { }
        expected(unexpected<E> const& error): kind_{kind::of_error}, error_{error.value()} { }
        expected(unexpected<E>&& error): kind_{kind::of_error}, error_{std::move(error.value()} { }
        constexpr E& error() noexcept { return error_; }
        constexpr E const& error() noexcept { return error_; }
        constexpr E&& error() noexcept { return error_; }
        constexpr bool has_value() const noexcept { return kind_ == kind::of_value; }
        explicit constexpr operator bool () const noexcept { return kind_ == kind::of_value; }
        
        
        ~expected() {
            switch(kind_) {
            case kind::of_value:
                return;
            case kind::of_error:
                error_.~E();
                return;
            }
        }
        
    private:
        enum class kind { of_value, of_error } kind_
        E error_;
    }; // expected
    
    
    enum class error {
        ok,
        file_size_is_too_small,
        invalid_file_signature,
        mismatch_file_size,
        mismatch_item_size,
        duplicated_key,
        storage_is_full
    }; // error


    class error_category : public std::error_category {

        char const* name() const noexcept override {
            return "persia";
        }

        std::string message(int code) const noexcept override {
            switch(error(code)) {
            case error::ok:
                return "Ok";
            case error::file_size_is_too_small:
                return "Storage file is too small";
            case error::invalid_file_signature:
                return "Invalid storage file signature";
            case error::mismatch_file_size:
                return "Mismatch file size";
            case error::mismatch_item_size:
                return "Mismatch item size";
            case error::duplicated_key:
                return "Duplicated key";
            case error::storage_is_full:
                return "Storage is full";
            default:
                return "Unknown";
            }
        }
    };


    inline error_category const persia_category;


    inline std::error_code make_error_code(error e) noexcept {
        return {int(e), persia_category};
    }
    
} // namespace persia


namespace std {

    template <> struct is_error_code_enum<persia::error> : true_type {};

} // std


namespace persia {
    
    using storage_index = std::uint32_t;
    
    namespace detail {
        
        
        class mapped_file {
        private:
            void* address_{nullptr};
            std::size_t size_{0};
#if defined(_WIN32)
            HANDLE file_{INVALID_HANDLE_VALUE};
            HANDLE mapping_{NULL};
#else
            int file_{-1};
#endif
        public:
        
            using size_type = std::size_t;
                        
            static expected<mapped_file, std::error_code>
            create(std::filesystem::path const& path) noexcept {
#if defined(_WIN32)
                auto file = ::CreateFileA(path.string().data(),
                                          GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if(file == INVALID_HANDLE_VALUE)
                    return make_unexpected({std::system_category(), ::GetLastError()});
                auto size = ::GetFileSize(file, NULL);
                constexpr auto PAGE_READWRITE = 0x04;
                auto mapping = ::CreateFileMapping(file, NULL, PAGE_READWRITE, 0, 0, NULL);
                if(mapping == NULL) {
                    auto const code = ::GetLastError();
                    ::CloseHandle(file);
                    return make_unexpected({std::system_category(), code});
                }
                auto address = ::MapViewOfFile(mapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
                return {mapped_file{address, size, file, mapping}};
#else
                auto file = open(path.string().data(), O_RDWR);
                if(file == -1)
                    return make_unexpected({std::system_category(), errno});
                struct stat sb;
                if(fstat(file, &sb) == -1) {
                    auto const code = errno;
                    ::close(file);
                    return make_unexpected({std::system_category(), code});
                }
                auto size = sb.st_size;
                auto address = ::mmap(NULL, size, PROT_RDWR, MAP_PRIVATE, file, 0);
                return {mapped_file{address, size, file}};
#endif
            }
            
            
            mapped_file() noexcept = default;
            mapped_file(mapped_file const&) = delete;
            mapped_file& operator = (mapped_file const&) = delete;
            
            
            mapped_file(mapped_file&& other) noexcept:
                address_{other.address_}, size_{other.size_}, file_{other.file_}
#if defined(_WIN32)
                , mapping_{other.mapping_}
#endif
            {
                other.address_ = nullptr;
                other.size_ = 0;
#if defined(_WIN32)
                other.file_ = INVALID_HANDLE_VALUE;
                other.mapping_ = NULL;
#else
                other.file_ = -1;
#endif
            }
            
            
            mapped_file& operator = (mapped_file&& other) noexcept {
                dispose();
                address_ = other.address_;
                other.address_ = nullptr;
                size_ = other.size_;
                other.size_ = 0;
                file_ = other.file_;
                other.file_ = 
#if defined(_WIN32)
                other.file_ = INVALID_HANDLE_VALUE;
                mapping_ = other.mapping_;
                other.mapping_ = NULL;
#else
                other.file_ = -1;
#endif
                
                return *this;
            }
            
            
            ~mapped_file() {
                dispose();
            }
            
            
            explicit operator bool () const noexcept {
                return address_ != nullptr;
            }
            
            
            void* address() noexcept {
                return address_;
            }
            
            
            T* as(size_type offset) noexcept {
                auto* bytes = static_cast<char*>(address_) + offset;
                return reinterpret_cast<T*>(bytes);
            }
            
            
            size_type size() const noexcept {
                return size_;
            }
            
        private:
        
#if defined(_WIN32)
            mapped_file(void* address, size_type size, HANDLE file, HANDLE mapping) noexcept:
                address_{address}, size_{size}, file_{file}, mapping_{mapping} { }
#else
            mapped_file(void* address, size_type size, int file) noexcept:
                address_{address}, size_{size}, file_{file} { }
#endif
        
            void dispose() noexcept {
#if defined(_WIN32)
                if(address_ != nullptr)
                    ::UnmapViewOfFile(address);
                if(mapping_ != NULL)
                    ::CloseHandle(mapping_);
                if(file_ != INVALID_HANDLE_VALUE)
                    ::CloseHandle(file_);
#else
                if(address_ != nullptr)
                    ::munmap(address_, size_);
                if(file_ != -1)
                    ::close(file_);
#endif
            }

        }; // mapped_file
        
        
        struct header {
            constexpr static valid_signature = 0xDA1AF11Eu;
            
            char signature[4];
            std::uint32_t item_size{0};
            std::uint32_t capacity{0};
            std::uint32_t size{0};
        }; // header
        
        
        template<typename T > struct record {
            std::uint32_t previous;
            std::uint32_t next;
            T data;
        }; // record
        
        
        template<typename T> class storage {
        private:
        
            constexpr static auto first_occupied_index = storage_index(0u);
            constexpr static auto first_free_index = storage_index(1u);
            
            
            mapped_file file_;
            header* header_{nullptr};
            record<T>* records_{nullptr};
            
            template<class R, typename D> class basic_iterator {
            friend class storage;
            private:
                R* records_{nullptr};
                std::uint32_t index_{first_occupied_index};
                
                basic_iterator(R* records, std::uint32_t index) noexcept
                    : records_{records}, index_{index} { }
                
            public:
                basic_iterator() = delete;
                basic_iterator(basic_iterator const&) noexcept = default;
                basic_iterator& operator = (basic_iterator const&) noexcept = default;
                
                bool operator == (basic_iterator const&) const noexcept = default;
                bool operator != (basic_iterator const&) const noexcept = default;
                D& operator * () const noexcept { return records_[index_].data; }
                D* operator -> () const noexcept { return &records_[index_].data; }
                std::uint32_t index() const noexcept { return index_; }
                
                basic_iterator& operator ++ () noexcept {
                    index_ = records_[index_].next;
                    return *this;
                }
                
                basic_iterator operator ++ (int) noexcept {
                    auto current = *this;
                    index_ = records_[index_].next;
                    return current;
                }
            }; // basic_iterator
            
            
        public:
        
            using size_type = std::uint32_t;
            using const_iterator = basic_iterator<record<T> const, T const>;
            using iterator = basic_iterator<record<T>, T>
            
            
            static expected<storage, std::error_code>
            create(std::filesystem::path const& path,
                   size_type initial_capacity) {
                auto* file = std::fopen(path.string().data(), "w+b");
                if(file == nullptr)
                    return make_unexpected({std::system_category(), errno});
                std::fclose(file);
                namespace fs = std::filesystem;
                auto const storage_size = sizeof(header) + (initial_capacity + 2) * sizeof(record<T>);
                auto ec = std::error_code{};
                fs::resize_file(path, storage_size, ec);
                if(!!ec)
                    return make_unexpected(ec);
                auto maybe_mapped_file = mapped_file::create(path);
                if(!maybe_mapped_file)
                    return make_unexpected(maybe_mapped_file.error());
                auto* header_ptr = maybe_mapped_file->as<header>(0);
                header_ptr->signature[0] = 0xDA;
                header_ptr->signature[1] = 0x1A;
                header_ptr->signature[2] = 0xF1;
                header_ptr->signature[3] = 0x1E;
                header_ptr->item_size = sizeof(T);
                header_ptr->capacity = initial_capacity;
                header_ptr->size = 0;
                auto* records = maybe_mapped_file->as<record<T>>(sizeof(header_ptr));
                auto* first_occupied = records + first_occupied_index;
                first_occupied->previous = first_occupied_index;
                first_occupied->next = first_occupied_index;
                auto const last_free_index = initial_capacity + 1;
                auto* first_free = records + first_free_index;
                first_free->previous = last_free_index;
                auto* last_free = records + last_free_index;
                last_free->next = first_free_index;
                auto last_index = first_free_index;
                auto const second_free_index = first_occupied_index > first_free_index
                    ? first_occupied_index + 1 : first_free_index + 1;
                for(auto index = second_free_index; index != last_free_index + 1; ++index) {
                    auto* current = records + index;
                    auto* last = records + last_index;
                    current->previous = last_index;
                    last->next = index;
                    last_index = index;
                }
                return {storage{std::move(*maybe_mapped_file), header_ptr, records}};
            }
            
            
            static expected<storage, std::error_code>
            open(std::filesystem::path const& path,
                 size_type initial_capacity) {
                auto expected_file = mapped_file::create(path);
                if(!expected_file)
                    return make_unexpected(expected_file.error());
                if(expected_file->size() < sizeof(header) + 2 * sizeof(record<T>))
                    return make_unexpected(make_error_code(error::file_size_is_too_small));
                auto* header_ptr = expected_file->as<header>(0);
                if(header_ptr->signature[0] != 0xDA
                    || header_ptr->signature[1] != 0x1A
                    || header_ptr->signature[2] != 0xF1
                    || header_ptr->signature[3] != 0x1E)
                    return make_unexpected(make_error_code(error::invalid_file_signature));
                if(expected_file->size() != sizeof(header) + (header_ptr->capacity + 2) * sizeof(record<T>))
                    return make_unexpected(make_error_code(error::mismatch_file_size));
                if(sizeof(T) != header_ptr->item_size)
                    return make_unexpected(make_error_code(error::mismatch_item_size));
                if(initial_capacity > header_ptr->capacity)
                    return expand(path, std::move(*expected_file), header_ptr, initial_capacity);
                auto* records = expected_file->as<record<T>>(sizeof(header));
                return {storage{std::move(*expected_file), header_ptr, records}};
            }
            
        
            static expected<storage, std::error_code> attach(std::filesystem::path const& path,
                                                             size_type initial_capacity) {
                auto ec = std::error_code{};
                if(std::filesystem::exists(path, ec))
                    return open(path);
                if(!!ec)
                    return make_unexpected(ec);
                return create(path, initial_capacity);
            }


            storage() = delete;
            storage(storage const&) = delete;
            storage& operator = (storage const&) = delete;


            storage(storage&& other) noexcept
                : file_{std::move(other.file_)}
                , header_{other.header_}
                , records_{other.records_} {
                other.header_ = nullptr;
                other.records_ = nullptr;
            }
            
            
            storage& operator = (storage&& other) noexcept {
                file_ = std::move(other.file_);
                header_ = other.header_;
                other.header_ = nullptr;
                records_ = other.records_;
                other.records_ = nullptr;
                return *this;
            }
            
            
            size_type capacity() const noexcept {
                return header->capacity;
            }
            
            
            size_type size() const noexcept {
                return header->size;
            }
            
            
            const_iterator begin() const noexcept {
                return const_iterator{records_, records_[first_occupied_index].next};
            }
            
            
            const_iterator end() const noexcept {
                return const_iterator{records_, first_occupied_index};
            }
            
            
            iterator begin() noexcept {
                return iterator{records_, records_[first_occupied_index].next};
            }
            
            
            iterator end() noexcept {
                return iterator{records_, first_occupied_index};
            }
            
            
            storage_index add(T const& data) noexcept {
                if(!records_)
                    return 0u;
                auto* first_free = records_ + first_free_index;
                auto const index = first_free->next;
                if(index == first_free_index)
                    return 0u;
                auto* record = records_ + index;
                auto* next_free = records_ + record->next;
                next_free->previous = first_free_index;
                first_free->next = record->next;
                auto* first_occupied = records_ + first_occupied_index;
                auto* next_occupied = records_ + first_occupied->next;
                record->previous = first_occupied_index;
                record->next = first_occupied->next;
                next_occupied->previous = index;
                first_occupied->next = index;
                record->data = data;
                ++header_->size;
                return index;
            }
            
            
            T const& operator [] (storage_index index) const noexcept {
                return records_[index].data;
            }
            
            
            T& operator [](storage_index index) noexcept {
                return records_[index].data;
            }
            
            
            bool remove(storage_index index) noexcept {
                if(index == 0)
                    return false;
                auto* record = records_ + index;
                record->data = T{};
                auto* previous = records_ + record->previous;
                auto* next = records_ + record->next;
                previous->next = record->next;
                next->previous = record->previous;
                auto* first_free = records_ + first_free_index;
                auto* next_free = records_ + first_free->next;
                record->next = first_free->next;
                record->previous = first_free_index;
                next_free->previous = index;
                first_free->next = index;
                --header_->size;
                return true;
            }


        private:
        
            storage(mapped_file file, header* header, record<T>* records) noexcept
                : file_{std::move(file)}, header_{header}, records_{records} {
            }
            
            
            static expected<storage, std::error_code>
            expand(std::filesystem::path const& path,
                   mapped_file original_file,
                   header* header_ptr,
                   size_type initial_capacity) {
                auto const original_capacity = header_ptr->capacity;
                original_file = mapped_file{};
                namespace fs = std::filesystem;
                auto const storage_size = sizeof(header) + (initial_capacity + 2) * sizeof(record<T>);
                auto ec = std::error_code{};
                fs::resize_file(path, storage_size, ec);
                if(!!ec)
                    return make_unexpected(ec);
                auto expected_file = mapped_file::create(path);
                if(!expected_file)
                    return make_unexpected(expected_file.error());
                header_ptr  = expected_file->as<header>(0);
                auto* records = expected_file->as<record<T>>(sizeof(header));
                auto* first_free = records + first_free_index;
                for(auto index = original_capacity + 2; index != initial_capacity + 2; ++index) {
                    auto* current = records + index;
                    auto* last = records + first_free->previous;
                    current->next = first_free_index;
                    current->previous = first_free->previous;
                    last->next = index;
                    first_free->previous = index;
                }
                return {storage{std::move(*expected_file), header_ptr, records}};
            }
        }; // storage
        
        
    } // namespace detail
    
    
    template<typename Key,
             typename Value,
             class Adapter = Value,
             class IndicesMap = std::unordered_map<Key, storage_index>>
    class map {
    private:
        using storage = detail::storage<Value>;

        IndicesMap indices_;
        storage storage_;
        
        
        template<class I, class S, class D> class basic_iterator {
        friend class map;
        private:
            I index_it_;
            S* storage_;
            
            basic_iterator(I index_it, S* storage) noexcept
                : index_it_{index_it}, storage_{storage} { }
        
        public:
            
            basic_iterator() = delete;
            basic_iterator(basic_iterator const&) noexcept = default;
            basic_iterator& operator = (basic_iterator const&) noexcept = default;
            
            bool operator == (basic_iterator const&) const noexcept = default;
            bool operator != (basic_iterator const&) const noexcept = default;
            D& operator * () const noexcept { return *(*storage_)[index_it_->second]; }
            D* operator -> () const noexcept { return (*storage_)[index_it_->second]; }
            
            basic_iterator& operator ++ () noexcept {
                ++index_it_;
                return *this;
            }
            
            basic_iterator operator ++ (int) noexcept {
                auto current = *this;
                ++index_it_;
                return current;
            }
        };

    public:

        using size_type = typename storage::size_type;
        using value_type = Value;
        using key_type = Key;
        using adapter_type = Adapter;
        using indices_type = IndicesMap;
        using const_iterator = basic_iterator<typename IndicesMap::const_iterator,
                                              detail::storage<Value> const,
                                              Value const>;
        using iterator = basic_iterator<typename IndicesMap::iterator,
                                        detail::storage<Value>,
                                        Value>;


        static expected<map, std::error_code> create(std::filesystem::path const& path,
                                                     size_type initial_capacity) {
            auto maybe_storage = storage::create(path, initial_capacity);
            if(!maybe_storage)
                return make_unexpected(maybe_storage.error());
            
            return {map{IndicesMap{}, std::move(*maybe_storage)}};
        }
        
        
        static expected<map, std::error_code> open(std::filesystem::path const& path,
                                                   size_type initial_capacity) {
            auto maybe_storage = storage::open(path, initial_capacity);
            if(!maybe_storage)
                return make_unexpected(maybe_storage.error());
            
            auto indices = IndicesMap{};
            indices.reserve(maybe_storage->size());
            for(auto it = maybe_storage->begin(), end = maybe_storage->end(); it != end; ++it)
                indices[Adapter::key_of(*it)] = it.index();
            
            return {map{std::move(indices), std::move(*maybe_storage)}};
        }

        
        map() = delete;
        map(map const&) = delete;
        map& operator = (map const&) = delete;
        map(map&& other) noexcept = default;
        map& operator = (map&& other) noexcept = default;
        
        
        size_type capacity() const noexcept {
            return storage_.capacity();
        }
        
        
        size_type size() const noexcept {
            return size_type(indices_.size());
        }
        
        
        bool empty() const noexcept {
            return indices_.empty();
        }
        
        
        const_iterator begin() const noexcept {
            return const_iterator{indices_.begin(), &storage_};
        }
        
        
        const_iterator end() const noexcept {
            return const_iterator{indices_.end(), &storage_};
        }
        
        
        iterator begin() noexcept {
            return iterator{indices_.begin(), &storage_};
        }
        
        
        iterator end() noexcept {
            return iterator{indices_.end(), &storage_};
        }
        
        
        expected<void, std::error_code> insert(Value const& value) {
            auto const key = Adapter::key_of(value);
            auto emplaced = indices_.try_emplace(key, 0u);
            if(!emplaced.second)
                return make_unexpected(make_error_code(error::duplicated_key));
            auto const index = storage_.add(value);
            if(index == 0u)
                return make_unexpected(make_error_code(error::storage_is_full));
            emplaced.first->second = index;
            return {};
        }
        
        
        expected<void, std::error_code> insert_or_assign(Value const& value) {
            auto const key = Adapter::key_of(value);
            auto emplaced = indices_.try_emplace(key, 0u);
            if(emplaced.second) {
                auto const index = storage_.add(value);
                if(index == 0u)
                    return make_unexpected(make_error_code(error::storage_is_full));
                emplaced.first->second = index;
            } else {
                auto const index = emplaced.first->second;
                storage_[index] = value;
            }
            return {};
        }
        
        
        bool erase(Key const& key) noexcept {
            auto index_found = indices_.find(key);
            if(index_found == indices_.end())
                return false;
            auto const index = index_found->second;
            storage_.remove(index);
            indices_.erase(index_found);
            return true;
        }
        
        
        Value const* find(Key const& key) const noexcept {
            auto const index_found = indices_.find(key);
            if(index_found == indices_.end())
                return nullptr;
            auto const index = index_found->second;
            return &storage_[index];
        }
        
        
        Value* find(Key const& key) noexcept {
            auto const index_found = indices_.find(key);
            if(index_found == indices_.end())
                return nullptr;
            auto const index = index_found->second;
            return &storage_[index];
        }
        
    private:
    
        map(IndicesMap&& indices, detail::storage&& storage) noexcept
            : indices_{std::move(indices)}, storage_{std::move(storage)} {
        }
            
    }; // map
    
    
    
} // namespace persia
