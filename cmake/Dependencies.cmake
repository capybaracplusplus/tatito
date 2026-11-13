include_guard(GLOBAL)

include(FetchContent)

# Avoid re-fetching moving refs after the first successful configure. Every
# source dependency below is pinned to a release archive or immutable commit.
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# Some transitive dependency releases still declare compatibility with old
# CMake versions. CMake 4 requires an explicit policy floor for those projects.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
        URL_MD5 e155202b2a589137f6804724bd182f12
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JWT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(JWT_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(JWT_ENABLE_COVERAGE OFF CACHE BOOL "" FORCE)
set(JWT_ENABLE_FUZZING OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        jwt-cpp
        GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
        GIT_TAG b0ea29a58fc852a67d4e896d266880c2c63b0c4c
)
FetchContent_MakeAvailable(jwt-cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(HIREDIS REQUIRED IMPORTED_TARGET hiredis>=1.0)

set(REDIS_PLUS_PLUS_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(REDIS_PLUS_PLUS_BUILD_ASYNC_TEST OFF CACHE BOOL "" FORCE)
set(REDIS_PLUS_PLUS_BUILD_ASYNC OFF CACHE STRING "" FORCE)
set(REDIS_PLUS_PLUS_BUILD_CORO OFF CACHE BOOL "" FORCE)
set(REDIS_PLUS_PLUS_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(REDIS_PLUS_PLUS_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(REDIS_PLUS_PLUS_BUILD_STATIC_WITH_PIC ON CACHE BOOL "" FORCE)
set(REDIS_PLUS_PLUS_CXX_STANDARD 20 CACHE STRING "" FORCE)
set(REDIS_PLUS_PLUS_USE_TLS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        redis-plus-plus
        GIT_REPOSITORY https://github.com/sewenew/redis-plus-plus.git
        GIT_TAG 0c4ba0370e46c3d0d443c0368d02195faa791219
)
FetchContent_MakeAvailable(redis-plus-plus)

find_package(Boost 1.82 CONFIG REQUIRED COMPONENTS log)
find_package(Threads REQUIRED)
