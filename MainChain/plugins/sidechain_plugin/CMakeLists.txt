file(GLOB HEADERS "include/eosio/sidechain_plugin/*.hpp")
add_library( sidechain_plugin
        sidechain_plugin.cpp
        ${HEADERS} )

target_link_libraries( sidechain_plugin chain_plugin eosio_chain appbase )
target_include_directories( sidechain_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
