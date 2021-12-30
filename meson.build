project('jaffar', 'c', 'cpp',
  version: '1.1.0',
  license: 'GPL-3.0-only',
  default_options : ['cpp_std=c++17', 'default_library=static', 'buildtype=release'],
)

cc = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')

jaffar_flags = [ ]
pymodule = import('python')
python = pymodule.find_installation('python3', required: true)

res = run_command(python, '-c', 'import os; print(os.environ["JAFFAR_DISABLE_MOVE_HISTORY"])')
if res.returncode() == 0
  #jaffar_flags += '-D_MAX_MOVELIST_SIZE='+res.stdout().strip()
  value = res.stdout().strip()
  if value == 'TRUE'
   jaffar_flags += '-DJAFFAR_DISABLE_MOVE_HISTORY'
  endif
endif


inc = include_directories([
 'extern/metrohash64',
 'extern/argparse',
 'extern/json/single_include',
 'extern/cbuffer',
 'extern/miniPoP',
 'extern/phmap'
 ])


jaffarFiles = [
  'source/miniPoPInstance.cc',
  'source/state.cc',
  'source/rule.cc',
  'source/frame.cc',
  'source/utils.cc',
  'extern/metrohash64/metrohash64.cc',
  'extern/miniPoP/miniPoP.c'
]

default_library='static'
default_library='shared'
sdl2_dep = dependency('sdl2')
sdl2_image_dep = dependency('SDL2_image')

deps = [
  sdl2_dep,
  sdl2_image_dep,
  dependency('openmp', required: true)
]

executable('jaffar-train',
  'source/train.cc',
  jaffarFiles,
  dependencies: deps,
  include_directories: inc,
  link_args: [ ],
  c_args: [ '-Wno-implicit', '-w', '-Wfatal-errors'],
  cpp_args: [ '-Wfatal-errors', '-funroll-loops'] + jaffar_flags
  )
  
executable('jaffar-tester',
  'source/tester.cc',
  jaffarFiles,
  dependencies: deps,
  include_directories: inc,
  link_args: [ ],
  c_args: [ '-Wno-implicit', '-w', '-Wfatal-errors' ],
  cpp_args: [ '-Wfatal-errors' ] + jaffar_flags
  )

checkStyleCommand = find_program('./tools/check_style.sh', required: true)
test('C++ Style check', checkStyleCommand)
