project('libsdlpop', 'c', 'cpp',
  default_options : ['cpp_std=c++1z', 'b_ndebug=if-release'],
)

cc = meson.get_compiler('c')
inc = include_directories([ 'extern/SDLPoP/src', 'extern/metrohash64', 'extern/json' ])

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
  'source/utils.cc',
  'source/search.cc'
]

deps = [
  dependency('sdl2'),
  dependency('SDL2_image'),
  dependency('mpich', required : false),
  dependency('mpi', required : false),
  cc.find_library('m', required : false),
]

metroHash64Lib = library('metroHash64Lib',
  metrohash64Files)
  
sdlPopLib = library('sdlPopLib',
  sdlPopFiles,
  dependencies : deps)
  
linkFlags = [
  '-ldl',
  '-fopenmp'
  ]
  
cppFlags = [
  '-Wfatal-errors',
  '-fopenmp',
  ]

executable('jaffar',
  'source/main.cc',
  jaffarFiles,
  dependencies: deps,
  include_directories: inc,
  link_with: [ sdlPopLib, metroHash64Lib ],
  link_args: linkFlags,
  cpp_args: cppFlags)
