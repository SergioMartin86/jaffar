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

res = run_command(python, '-c', 'import os; print(os.environ["JAFFAR_MAX_MOVELIST_SIZE"])')
if res.returncode() == 0
  jaffar_flags += '-D_MAX_MOVELIST_SIZE='+res.stdout().strip()
endif

res = run_command(python, '-c', 'import os; print(os.environ["JAFFAR_MAX_RULE_COUNT"])')
if res.returncode() == 0
  jaffar_flags += '-D_MAX_RULE_COUNT='+res.stdout().strip()
endif

res = run_command(python, '-c', 'import os; print(os.environ["JAFFAR_MAX_FRAME_DIFF"])')
if res.returncode() == 0
  jaffar_flags += '-D_MAX_FRAME_DIFF='+res.stdout().strip()
endif

inc = include_directories([
 'extern/metrohash64',
 'extern/argparse',
 'extern/json/single_include',
 'extern/cbuffer',
 'extern/miniPoP'
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
abs_base = subproject('abseil').get_variable('absl_base_dep')
abs_cont = subproject('abseil').get_variable('absl_container_dep')
default_library='shared'
sdl2_dep = dependency('sdl2')
sdl2_image_dep = dependency('sdl2_image')
boost_dep = dependency('boost')

deps = [
  sdl2_dep,
  sdl2_image_dep,
  dependency('openmp', required: true),
  abs_base,
  abs_cont,
  boost_dep
]

executable('jaffar-train',
  'source/train.cc',
  jaffarFiles,
  dependencies: deps,
  include_directories: inc,
  link_args: [ '-ldl' ],
  cpp_args: [ '-Wfatal-errors' ] + jaffar_flags
  )

checkStyleCommand = find_program('./tools/check_style.sh', required: true)
test('C++ Style check', checkStyleCommand)
