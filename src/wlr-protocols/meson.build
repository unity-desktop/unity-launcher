wayland_scanner = find_program('wayland-scanner')
foreign_toplevel_xml = files('wlr-foreign-toplevel-management-unstable-v1.xml')

unity_launcher_sources += files(
  'unity-wlr-toplevel.c',
  'unity-wlr-toplevels.c',
)

unity_launcher_sources += custom_target(
  'foreign-toplevel-client-header',
  input: foreign_toplevel_xml,
  output: '@BASENAME@.h',
  command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
)

unity_launcher_sources += custom_target(
  'foreign-toplevel-private-code',
  input: foreign_toplevel_xml,
  output: '@BASENAME@.c',
  command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
)
