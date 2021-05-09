project('jaffar', 'c', 'cpp',
  version: '1.1.0',
  license: 'GPL-3.0-only',
  default_options : ['cpp_std=c++17', 'default_library=shared', 'buildtype=release', 'c_args=-DSDL_VIDEO_DRIVER_DUMMY'],
)

cc = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')

inc = include_directories([
 'extern/SDLPoP/src',
 'extern/metrohash64',
 'extern/argparse'
  ])

sdlPopFiles = [
  'extern/SDLPoP/src/data.c',
  'extern/SDLPoP/src/lighting.c',
  'extern/SDLPoP/src/menu.c',
  'extern/SDLPoP/src/midi.c',
  'extern/SDLPoP/src/opl3.c',
  'extern/SDLPoP/src/options.c',
  'extern/SDLPoP/src/replay.c',
  'extern/SDLPoP/src/screenshot.c',
  'extern/SDLPoP/src/seg000.c',
  'extern/SDLPoP/src/seg001.c',
  'extern/SDLPoP/src/seg002.c',
  'extern/SDLPoP/src/seg003.c',
  'extern/SDLPoP/src/seg004.c',
  'extern/SDLPoP/src/seg005.c',
  'extern/SDLPoP/src/seg006.c',
  'extern/SDLPoP/src/seg007.c',
  'extern/SDLPoP/src/seg008.c',
  'extern/SDLPoP/src/seg009.c',
  'extern/SDLPoP/src/seqtbl.c',
  'extern/SDLPoP/src/stb_vorbis.c'
]

metrohash64Files = [
  'extern/metrohash64/metrohash64.cc',
]

jaffarFiles = [
  'source/SDLPopInstance.cc',
  'source/state.cc',
  'source/rule.cc',
  'source/frame.cc',
  'source/utils.cc'
]

sdl2_dep = dependency('sdl2')
sdl2_image_dep = dependency('sdl2_image')

mpi_dep = dependency('mpi', language: 'cpp', required: false)  
if mpi_dep.found() == false
 mpi_dep = dependency('mpich', required: false)
endif

boost_dep = dependency('boost')

deps = [
  mpi_dep,
  boost_dep,
  dependency('openmp', required: true),
  dependency('sdl2'),
  dependency('sdl2_image'),
  subproject('json').get_variable('nlohmann_json_dep'),
  subproject('abseil').get_variable('absl_base_dep'),
  subproject('abseil').get_variable('absl_container_dep')
]

metroHash64Lib = library('metroHash64Lib',
  metrohash64Files)
  
sdlPopLib = library('sdlPopLib',
  sdlPopFiles,
  c_args: [ '-DDISABLE_ALL_FIXES' ],
  dependencies : deps)
  
executable('jaffar-train',
  'source/train.cc',
  jaffarFiles,
  dependencies: deps,
  include_directories: inc,
  link_with: [ sdlPopLib, metroHash64Lib ],
  link_args: [ '-ldl' ],
  cpp_args: [ '-Wfatal-errors' ]
  )
  
executable('jaffar-play',
  'source/play.cc',
  jaffarFiles,
  dependencies: deps,
  include_directories: inc,
  link_with: [ metroHash64Lib ],
  link_args: [ '-ldl', '-lncurses' ],
  cpp_args: [ '-Wfatal-errors' ]
  )
  
executable('jaffar-show',
  'source/show.cc',
  jaffarFiles,
  dependencies: deps,
  include_directories: inc,
  link_with: [ sdlPopLib, metroHash64Lib ],
  link_args: [ '-ldl' ],
  cpp_args: [ '-Wfatal-errors' ]
  )

checkStyleCommand = find_program('./tools/check_style.sh', required: true)
test('C++ Style check', checkStyleCommand)
