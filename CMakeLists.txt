cmake_minimum_required(VERSION 3.28)

project(Tic_Tac_Toe_Boost VERSION 0.1.0 LANGUAGES C CXX)

include(cmake/Dependencies.cmake)
include(cmake/CompilerWarnings.cmake)
include(cmake/Sanitizers.cmake)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
        src/main.cpp
        src/handlers/base_handler.cpp
        src/handlers/not_found_handler.cpp
        src/handlers/notify_handler.cpp
        src/handlers/game/create_game_handler.cpp
        src/handlers/game/join_game_handler.cpp
        src/handlers/game/remove_game_handler.cpp
        src/handlers/move/make_move_handler.cpp
        src/repositories/game_repository.cpp
        src/repositories/matchmaking_repository.cpp
        src/routes/route_registry.cpp
        src/routes/router.cpp
        src/routes/routes.cpp
        src/services/game_service.cpp
        src/services/matchmaking_service.cpp
        src/services/move_service.cpp
        src/services/notification_service.cpp
        src/session/session.cpp
        src/session_manager/session_manager.cpp
        src/tcp_server/tcp_server.cpp
        src/utils/http_request.cpp
        src/utils/http_response.cpp
        src/utils/jwt_token.cpp
        src/utils/redis_client_factory.cpp
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(${PROJECT_NAME} PRIVATE
        Boost::headers
        Boost::log
        Threads::Threads
        jwt-cpp::jwt-cpp
        nlohmann_json::nlohmann_json
        redis++_static
        PkgConfig::HIREDIS
)

tatito_set_compiler_warnings(${PROJECT_NAME})
tatito_enable_sanitizers(${PROJECT_NAME})
