gtkmdview_src = [
  'mdview_internal.c',
  'gtkmdview.c',
  'entity.c',
]

gtkmdview_deps = [
  gtk_dep,
  md4c_dep
]

gtkmdview = shared_library(
  'gtkmdview',
  gtkmdview_src,
  dependencies: gtkmdview_deps,
  install: true,
  soversion: meson.project_version(),
)

install_headers(['gtkmdview.h'])

pkg.generate(libraries: [gtkmdview] + gtkmdview_deps,
             version: meson.project_version(),
             name: 'gtkmdview',
             description : 'A simple library that renders markdown as a GtkWidget for viewing')
