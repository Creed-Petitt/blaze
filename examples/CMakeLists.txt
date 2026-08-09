# examples/CMakeLists.txt

file(GLOB EXAMPLE_SOURCES "*.cpp")
set(BLAZE_POSTGRES_EXAMPLES
    02_todo_crud
    07_advanced_queries
    11_transactions
)

foreach(source_file ${EXAMPLE_SOURCES})
    get_filename_component(example_name ${source_file} NAME_WE)

    if(example_name IN_LIST BLAZE_POSTGRES_EXAMPLES AND NOT TARGET blaze::postgres)
        message(STATUS "Skipping ${example_name}: requires blaze::postgres")
        continue()
    endif()
    
    add_executable(${example_name} ${source_file})
    
    target_link_libraries(${example_name} PRIVATE blaze::core)
    
    if(example_name IN_LIST BLAZE_POSTGRES_EXAMPLES)
        target_link_libraries(${example_name} PRIVATE blaze::postgres)
    endif()
    
    # Enable C++20
    target_compile_features(${example_name} PRIVATE cxx_std_20)

    # Set output directory to examples/
    set_target_properties(${example_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
    )
endforeach()
