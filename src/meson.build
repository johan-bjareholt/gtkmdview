md2gtktextbuf_src = [
  'mdview_internal.c',
  'gtkmdview.c',
  'entity.c',
]

md2gtktextbuf_deps = [
  gtk_dep,
  md4c_dep
]

md2gtktextbuf = shared_library(
  'md2gtktextbuf',
  md2gtktextbuf_src,
  dependencies: md2gtktextbuf_deps,
  install: true,
  soversion: meson.project_version(),
)

install_headers(['gtkmdview.h'])

pkg.generate(libraries: [md2gtktextbuf] + md2gtktextbuf_deps,
             version: meson.project_version(),
             name: 'md2gtktextbuf',
             description : 'A simple library that converts markdown to a GtkTextBuffer')
