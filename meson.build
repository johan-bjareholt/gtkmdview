project('gtkmdview', 'c',
  version: '4.0',
  meson_version: '>=0.49',
  default_options: ['warning_level=1', 'werror=true'])

gnome = import('gnome')
pkg = import('pkgconfig')

gtk_dep = dependency('gtk4', version : '>=4.0')
md4c_dep = dependency('md4c')

add_global_link_arguments(['-Wall'], language: 'c')

subdir('src')
subdir('examples')
