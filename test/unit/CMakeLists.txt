pcp_add_test_executable(unit_tests
  epoch.c
  ip.c
  layout.c
  layout_c11.c
  main.c
  schedule.c
  session.c
  wire.c)

target_include_directories(unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/lib)

# The public headers have to work for C11 users, and agree with the C23 library
# on the layout of their types. Per-file options come after the target's
# -std=, so they win. Pedantic, since GCC accepts some C23 in C11 as an
# extension.
set_source_files_properties(layout_c11.c
  PROPERTIES COMPILE_OPTIONS "-std=c11;-pedantic-errors")
