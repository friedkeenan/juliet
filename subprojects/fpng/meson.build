project('fpng', 'cpp',
    default_options : [
        'buildtype=release',

        # Notably, this will prevent STL assertions as well.
        'b_ndebug=if-release',
    ]
)

cpp_args = [
    '-std=gnu++2c',
]

if build_machine.cpu_family() in ['x86_64', 'x84']
    cpp_args += [
        '-msse4.1',
        '-mpclmul',
    ]
endif

lib = static_library(meson.project_name(),
    include_directories : 'include',
    sources : 'source/fpng.cpp',

    cpp_args : cpp_args,
)

fpng_dep = declare_dependency(
    include_directories : 'include',
    link_with : lib,
)
