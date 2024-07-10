include(CTest)

# Test dependencies interface library
add_library(test_dependencies INTERFACE)
target_link_libraries(test_dependencies INTERFACE
    Unity
    zsf-static
)
target_compile_definitions(test_dependencies INTERFACE ZSF_STATIC)
set_target_properties(test_dependencies PROPERTIES C_STANDARD 23)

# Test executables and add them to CTest
add_executable(test_dimr_bmi test_dimr_bmi.c)
target_link_libraries(test_dimr_bmi PRIVATE test_dependencies)
add_test(test_dimr_bmi test_dimr_bmi)

add_executable(test_io_layer_distribution test_io_layer_distribution.c)
target_link_libraries(test_io_layer_distribution PRIVATE test_dependencies)
add_test(test_io_layer_distribution test_io_layer_distribution)

add_executable(test_load_csv test_load_csv.c)
target_link_libraries(test_load_csv PRIVATE test_dependencies)
add_test(test_load_csv test_load_csv)
