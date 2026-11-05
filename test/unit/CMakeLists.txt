pcp_add_test_executable(unit_tests
  epoch.c
  ip.c
  main.c
  schedule.c
  session.c
  wire.c)

target_include_directories(unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/lib)
