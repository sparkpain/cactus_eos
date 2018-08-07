file(GLOB HEADERS "include/eosio/sync_plugin/*.hpp")
add_library( sync_plugin
        sync_plugin.cpp
        ${HEADERS} )

target_link_libraries( sync_plugin client_plugin chain_plugin eosio_chain appbase )
target_include_directories( sync_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
