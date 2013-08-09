find_package(Apache HINTS $ENV{MISC_CMAKE_MODULES} REQUIRED)
include_directories(. ${ApacheINCLUDE_DIR} ${ApacheAPR_INCLUDE_DIR} ${LibBitForge_INCLUDE_DIR})
add_definitions(-fvisibility=default)

find_library(LIBAPR NAMES apr-1)
find_library(LIBAPRUTIL NAMES aprutil-1)

if (NOT NO_USE_AIO)
    add_definitions(-DUSE_AIO)
    set(AIO_LIBRARIES rt)
endif()

add_library(mod_cxx MODULE
    mod_cxx.cpp
    config.cpp
    handlers/handlerfactory.cpp
    handlers/apachehandler.cpp
)

get_target_property(VAR mod_cxx property)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    get_target_property(MOD_LINK_FLAGS mod_cxx LINK_FLAGS)

    if (DEFINED MOD_LINK_FLAGS)
        set_target_properties(mod_cxx PROPERTIES LINK_FLAGS "-bundle_loader /usr/sbin/httpd ")
    else()
        set_target_properties(mod_cxx PROPERTIES LINK_FLAGS "${MOD_LINK_FLAGS} -bundle_loader /usr/sbin/httpd ")
    endif()
endif()

target_link_libraries(mod_cxx ${LIBAPR} ${LIBAPRUTIL} ${AIO_LIBRARIES} ${LibBitForge_LIBRARIES})

set_target_properties(mod_cxx PROPERTIES PREFIX "")

get_property(output TARGET mod_cxx PROPERTY LOCATION)
add_custom_command(TARGET mod_cxx POST_BUILD COMMAND strip ${STRIP_ARGS} ${output} COMMENT Stripping)

foreach(i "/usr/lib/apache2/modules" "/etc/httpd/modules/" "/usr/lib64/httpd/modules/" "/usr/local/modules/")
    if(IS_DIRECTORY ${i})
        set(OUTPUT_DIR ${i})
        break()
    endif()
endforeach()
install(TARGETS mod_cxx LIBRARY DESTINATION ${OUTPUT_DIR})

install(FILES
    mod_cxx.h
    globals.h
    handlers.h
    handlers_helpers.h
    DESTINATION include/mod_cxx)

if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    install(CODE "execute_process(COMMAND /usr/local/sbin/apachectl restart)")
else()
    if(EXISTS /etc/fedora-release)
        install(CODE "execute_process(COMMAND sudo systemctl restart httpd.service)")
    else()
        install(CODE "execute_process(COMMAND sudo /etc/init.d/apache2 restart)")
    endif()
endif()

if(EXISTS "/etc/slackware-version")
    message(STATUS "I'm in a slackware")

    add_custom_command(TARGET mod_cxx POST_BUILD COMMAND  mkdir "-p" "${CMAKE_HOME_DIRECTORY}/package/slackware/modcxx-0.1-x86_64-1/usr/lib64/httpd/modules" COMMENT "Creating slackware package dirs")
    add_custom_command(TARGET mod_cxx POST_BUILD COMMAND  mkdir "-p" "${CMAKE_HOME_DIRECTORY}/package/slackware/modcxx-0.1-x86_64-1/usr/local/include/mod_cxx" COMMENT "Creating slackware package dirs")
    
    add_custom_command(TARGET mod_cxx POST_BUILD COMMAND  install ${MOD_CXX} "${CMAKE_HOME_DIRECTORY}/package/slackware/modcxx-0.1-x86_64-1/usr/lib64/httpd/modules" COMMENT "Installing mod_cxx to slackware package dir")
    
    add_custom_command(TARGET mod_cxx POST_BUILD COMMAND  install 
        mod_cxx.h
        globals.h
        handlers.h
        handlers_helpers.h
    "${CMAKE_HOME_DIRECTORY}/package/slackware/modcxx-0.1-x86_64-1/usr/local/include/mod_cxx" COMMENT "Installing mod_cxx headers")

    add_custom_command(TARGET mod_cxx POST_BUILD COMMAND makepkg -l y -c n ../modcxx-0.1-x86_64-1.tgz WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/package/slackware/modcxx-0.1-x86_64-1" COMMENT "Packaging mod_cxx")

endif()