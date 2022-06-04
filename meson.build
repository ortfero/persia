project('persia', 'cpp',
    license: 'MIT',
    default_options: [
        'cpp_std=c++17',
        'warning_level=3'])

headers = [
    'include/persia/storage.hpp'
]

incdirs = include_directories('./include')

persia = declare_dependency(
    include_directories: incdirs,
    sources: headers
)

# subdir('benchmark')
subdir('test')

install_headers(headers, subdir: 'persia')

pkg = import('pkgconfig')
pkg.generate(
    name: 'persia',
    filebase: 'persia',
    subdirs: ['persia'],
    description: 'C++ 17 single-header library for persistent data structures'
)
