# examples/CMakeLists.txt

file(GLOB EXAMPLE_SOURCES "*.cpp")

foreach(source_file ${EXAMPLE_SOURCES})
    get_filename_component(example_name ${source_file} NAME_WE)
    
    add_executable(${example_name} ${source_file})
    
    # Link against the blaze core and drivers
    target_link_libraries(${example_name} PRIVATE blaze_core)
    
    if(TARGET blaze_postgres)
        target_link_libraries(${example_name} PRIVATE blaze_postgres)
    endif()
    
    if(TARGET blaze_mysql)
        target_link_libraries(${example_name} PRIVATE blaze_mysql)
    endif()
    
    # Enable C++20
    target_compile_features(${example_name} PRIVATE cxx_std_20)
    
    # Add include directories
    target_include_directories(${example_name} PRIVATE 
        ${CMAKE_SOURCE_DIR}/framework/include
    )

    # Set output directory to examples/
    set_target_properties(${example_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
    )
endforeach()
