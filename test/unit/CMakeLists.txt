pcp_add_test_executable(unit_tests
  c11.c
  epoch.c
  ip.c
  main.c
  schedule.c
  session.c
  wire.c)

target_include_directories(unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/lib)

# The public headers have to work for C11 users. Per-file options come after the
# target's -std=, so they win. Pedantic, since GCC accepts some C23 in C11 as an
# extension.
set_source_files_properties(c11.c
  PROPERTIES COMPILE_OPTIONS "-std=c11;-pedantic-errors")
