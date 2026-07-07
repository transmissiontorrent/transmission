function(AddShowTest name file_basename)
    add_test(
        NAME ${name}
        COMMAND
            ${CMAKE_COMMAND}
            -D transmission_show:string=$<TARGET_FILE:transmission-show>
            -D torrent_file:string=assets/${file_basename}.torrent
            -D reference_file:string=${CMAKE_CURRENT_SOURCE_DIR}/assets/${file_basename}.show
            -P "${CMAKE_CURRENT_SOURCE_DIR}/run_transmission_show.cmake"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endfunction()

# disabled AddShowTest(transmission-show-bittorrent-v2 bittorrent-v2-test)
AddShowTest(transmission-show-bittorrent-v2-hybrid-test bittorrent-v2-hybrid-test)
AddShowTest(transmission-show-inner-sanctum Inner_Sanctum_movie_archive)
AddShowTest(transmission-show-thor Thor_and_the_Amazon_Women.avi)
AddShowTest(transmission-show-ubuntu ubuntu-20.04.3-desktop-amd64.iso)
AddShowTest(transmission-show-ubuntu-hybrid hybrid-single-ubuntu-20.04.3-desktop-amd64.iso)
AddShowTest(transmission-show-bitcomet-path.utf8 bitcomet.utf-8.special)

if(TARGET transmission-remote)
    include(GoogleTest)

    add_executable(transmission-remote-test remote-test.cc)

    set_property(
        TARGET transmission-remote-test
        PROPERTY FOLDER "tests")

    target_compile_definitions(transmission-remote-test
        PRIVATE
            __TRANSMISSION__
            TR_REMOTE_EXE="$<TARGET_FILE:transmission-remote>")

    target_link_libraries(transmission-remote-test
        PRIVATE
            ${TR_NAME}
            GTest::gtest_main
            transmission::fmt-header-only
            libevent::core
            libevent::extra)

    add_dependencies(transmission-remote-test transmission-remote)

    tr_setup_gtest_target(transmission-remote-test "UR.")
endif()
