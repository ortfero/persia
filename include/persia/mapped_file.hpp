// This file is part of persia library
// Copyright 2022 Andrei Ilin <ortfero@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once


#include <cstddef>
#include <filesystem>
#include <system_error>


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

        class expected;
        static expected create(std::filesystem::path const& path) noexcept;
        
        
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
        
        
        template<typename T> T* cast(size_type offset) noexcept {
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
                ::UnmapViewOfFile(address_);
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


    class mapped_file::expected {
    private:
        std::error_code error_code_;
        mapped_file file_;
            
    public:
        
        expected(std::error_code ec)
            : error_code_{ec} {
        }
        
        
        expected(mapped_file&& f)
            : file_(std::move(f)) {
        }
        
        
        explicit operator bool () const noexcept {
            return !!file_;
        }
        
        
        mapped_file& operator * () & noexcept {
            return file_;
        }
        
        
        mapped_file&& operator * () && noexcept {
            return std::move(file_);
        }
        
        
        mapped_file* operator -> () noexcept {
            return &file_;
        }
        
        
        std::error_code error() const noexcept {
            return error_code_;
        }
    }; // mapped_file::expected


    inline  mapped_file::expected mapped_file::create(std::filesystem::path const& path) noexcept {
#if defined(_WIN32)
        auto file = ::CreateFileA(path.string().data(),
                                  GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if(file == INVALID_HANDLE_VALUE)
            return {std::error_code{int(::GetLastError()), std::system_category()}};
        auto size = ::GetFileSize(file, NULL);
        //constexpr auto PAGE_READWRITE = 0x04;
        auto mapping = ::CreateFileMapping(file, NULL, PAGE_READWRITE, 0, 0, NULL);
        if(mapping == NULL) {
            auto const code = int(::GetLastError());
            ::CloseHandle(file);
            return {std::error_code{code, std::system_category()}};
        }
        auto* address = ::MapViewOfFile(mapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
        if(address == nullptr) {
            auto const code = int(::GetLastError());
            ::CloseHandle(file);
            return {std::error_code{code, std::system_category()}};
        }
        return {mapped_file{address, size, file, mapping}};
#else
        auto file = ::open(path.string().data(), O_RDWR);
        if(file == -1)
            return {std::error_code{errno, std::system_category()}};
        struct stat sb;
        if(::fstat(file, &sb) == -1) {
            auto const code = errno;
            ::close(file);
            return {std::error_code{code, std::system_category()}};
        }
        auto size = sb.st_size;
        auto* address = ::mmap(NULL, size, PROT_RDWR, MAP_PRIVATE, file, 0);
        if(address == MMAP_FAILED) {
            auto const code = errno;
            ::close(file);
            return {std::error_code{code, std::system_category()}};
        }
        return {mapped_file{address, size, file}};
#endif
    }
    
    
} // namespace persia
