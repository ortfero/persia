# persia

C++ 17 single-header library for persistent data structures


## Storage

### Features

* Automatic storage extending if requested capacity is greater then the actual capacity.


### Synopsis

```cpp
using storage_index = std::uint32_t;

template<typename Key,
         typename Value,
         class Adapter = Value,
         class Indices = std::unordered_map<K, storage_index>>
class storage {
public:
    using key_type = Key;
    using value_type = Value;
    using adapter_type = Adapter;
    using indices_type = Indices;
    using size_type = std::uint32_t;
    using const_iterator = /* implementation defined */;
    using iterator = /* implementation defined */;
    
    static expected<storage, std::error_code>
    create(std::filesystem::path const& path, size_type initial_capacity);
    
    static expected<storage, std::error_code>
    open(std::filesystem::path const& path, size_type initial_capacity);
    
    storage() = delete;
    storage(storage const&) = delete;
    storage& operator = (storage const&) = delete;
    storage(storage&&) noexcept = default;
    storage& operator = (storage&&) noexcept = default;
    
    size_type capacity() const noexcept;
    size_type size() const noexcept;
    bool empty() const noexcept;
    
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    iterator begin() noexcept;
    iterator end() noexcept;
    
    bool insert(Value const& value);
    bool insert_or_assign(Value const& value);
    
    Value const* find(Key const& key) const noexcept;
    Value* find(Key const& key) noexcept;
    
    bool erase(Key const& key) noexcept;
    void clear() noexcept;
};
```


### Snippets


#### Create storage

```cpp
#include <persia/storage.hpp>

struct data {
    int id;
    int value;
    
    static int key_of(data const& data) { return data.id; }
};

...

using storage = persia::storage<int, data>;

storage::expected expected_storage = storage::create("data.pmap", 8192);
if(!expected_storage) {
    std::cout << expected_storage.error().message() << '\n';
    return;
}
storage& storage = *expected_storage;
```


#### Create storage with adapter

```cpp
#include <persia/storage.hpp>
...
struct adapter {
    static int key_of(int x) { return x; }
};

using storage = persia::storage<int, int, adapter>;

auto expected_storage = storage::create("data.pmap", 8192);
if(!expected_storage) {
    std::cout << expected_storage.error().message() << '\n';
    return;
}
auto& storage = *expected_storage;
```


#### Open storage

```cpp

struct data {
    int id;
    int value;
    
    static int key_of(data const& data) { return data.id; }
};

using storage = persia::storage<int, data>;
auto expected_storage = storage::open("data.pmap", 8192);
if(!expected_storage) {
    std::cout << expected_storage.error().message() << '\n';
    return;
}
auto& storage = *expected_storage;
```


#### Insert new item in the storage

```cpp
...
bool const inserted =  storage.insert(data{-1, -1});
```

#### Insert or assign item

```cpp
...
bool const updated =  storage.insert_or_assign(data{-1, -1});
```


#### Update item

```cpp
...
data* p =  storage.find(-1);
if(!p) {
    std::cout << "Not found\n";
    return;
}
p->value = 1;
```


#### Erase item

```cpp
...
bool const erased = storage.erase(-1);
```



## Usage

Drop the contents of the `include` directory somewhere at your include path


## Tests

To run tests:

```shell
cd persia
mkdir build
cd build
meson ..
ninja
./test/persia-test
```
