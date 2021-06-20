project('md4c-gtk4', 'c',
  version: '1.0',
  meson_version: '>=0.49',
  warning_level: 2)

gnome = import('gnome')

gtk_dep = dependency('gtk4', version : '>=4.0')
md4c_dep = dependency('md4c')

add_global_link_arguments(['-Wall'], language: 'c')

subdir('src')
subdir('examples')
