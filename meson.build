project(
    'post',
    'c',
    default_options : ['warning_level=3', 'c_std=c11', 'werror=true'],
    meson_version : '>=1.1'
)

render_backend = get_option('render_backend')

srcs = files(
    'src/posix/proc.c',
    'src/app.c',
    'src/config.c',
    'src/font.c',
    'src/parser.c',
    'src/string.c',
)

if render_backend == 'sdl'
    srcs += files(
        'src/sdl/app.c',
        'src/sdl/main.c',
        'src/sdl/renderer.c',
    )
else
    error(f'unsupported render backend: \'@render_backend@\'')
endif

sdl_dep3 = dependency('sdl3')
fontconfig_dep = dependency('fontconfig')
freetype2_dep = dependency('freetype2')

post = executable(
    'post',
    srcs,
    dependencies : [ sdl_dep3, fontconfig_dep, freetype2_dep ],
    include_directories : [ 'include' ],
)

test(
    'post',
    post,
    timeout : 0,
)