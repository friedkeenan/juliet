project('juliet', 'cpp',
    default_options : [
        # -O3
        'buildtype=release',

        # Notably, this will prevent STL assertions as well.
        'b_ndebug=if-release',

        'cpp_rtti=false',
        'cpp_eh=none',
    ],
)

dependencies = [
    dependency('sfml-graphics'),

    # NOTE: fpng is separated out into a subproject because if
    # we compiled with all our warnings on, it would yell a lot.
    subproject('fpng').get_variable('fpng_dep'),
]

link_args = [
    '-s',
]

cpp_args = [
    '-std=gnu++2c',

    '-Wall',
    '-Wextra',
    '-Wpedantic',
    '-Wconversion',
    '-Werror',
]

sanitizers = [
    # '-fsanitize=address',
    # '-fsanitize=undefined',
]

cpp_args  += sanitizers
link_args += sanitizers

sources = []
subdir('source')

executable(meson.project_name(), sources,
    include_directories : 'include',
    dependencies        : dependencies,

    link_args : link_args,
    cpp_args  : cpp_args,
)
