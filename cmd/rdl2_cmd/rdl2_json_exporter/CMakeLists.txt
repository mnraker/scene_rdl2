# Copyright 2023-2024 DreamWorks Animation LLC
# SPDX-License-Identifier: Apache-2.0

set(target rdl2_json_exporter)

add_executable(${target})

target_sources(${target}
    PRIVATE
        rdl2_json_exporter.cc
)

target_link_libraries(${target}
    PRIVATE
        Boost::filesystem
        Boost::program_options
        JsonCpp::JsonCpp
        ${PROJECT_NAME}::render_logging
        ${PROJECT_NAME}::scene_rdl2
)

# Set standard compile/link options
SceneRdl2_cxx_compile_definitions(${target})
SceneRdl2_cxx_compile_features(${target})
SceneRdl2_cxx_compile_options(${target})
SceneRdl2_link_options(${target})

install(TARGETS ${target}
    RUNTIME DESTINATION bin)

# rdl2_json_exporter runs during the build and the runtime DLLs need to be next to it
# for windows to recognise and load them
if(IsWindowsPlatform)
    target_link_options(${target} PRIVATE /SUBSYSTEM:CONSOLE)
    add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${target}> $<TARGET_RUNTIME_DLLS:${target}>
    COMMAND_EXPAND_LISTS
    )
endif()
