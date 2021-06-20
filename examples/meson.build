example_deps = [
  gtk_dep,
  md4c_dep
]

executable(
  'gtkmdview_example',
  'gtkmdview.c',
  link_with: md2gtktextbuf,
  dependencies: example_deps,
  install: false,
  gui_app: true,
)
