project('libsdlpop', 'c', 'cpp',
  default_options : ['cpp_std=c++1z', 'b_ndebug=if-release'],
)

cc = meson.get_compiler('c')
inc = include_directories('extern/sdlpop')

sdlPopFiles = [
  'extern/sdlpop/data.c',
  'extern/sdlpop/lighting.c',
  'extern/sdlpop/menu.c',
  'extern/sdlpop/midi.c',
  'extern/sdlpop/opl3.c',
  'extern/sdlpop/options.c',
  'extern/sdlpop/replay.c',
  'extern/sdlpop/screenshot.c',
  'extern/sdlpop/seg000.c',
  'extern/sdlpop/seg001.c',
  'extern/sdlpop/seg002.c',
  'extern/sdlpop/seg003.c',
  'extern/sdlpop/seg004.c',
  'extern/sdlpop/seg005.c',
  'extern/sdlpop/seg006.c',
  'extern/sdlpop/seg007.c',
  'extern/sdlpop/seg008.c',
  'extern/sdlpop/seg009.c',
  'extern/sdlpop/seqtbl.c',
  'extern/sdlpop/stb_vorbis.c'
]

jaffarFiles = [
  'source/sdlpop-instance.cc',
]

deps = [
  dependency('sdl2'),
  dependency('SDL2_image'),
  cc.find_library('m', required : false),
]

sdlPopLib = library('sdlPopLib',
  sdlPopFiles,
  dependencies : deps)
  
linkFlags = [
  '-ldl',
  '-fopenmp'
  ]
  
cppFlags = [
  '-Wfatal-errors',
  '-fopenmp'
  ]

executable('jaffar',
  'source/main.cc',
  jaffarFiles,
  dependencies: deps,
  include_directories: inc,
  link_with: sdlPopLib,
  link_args: linkFlags,
  cpp_args: cppFlags)
