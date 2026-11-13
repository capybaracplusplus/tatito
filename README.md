# Tic-tac-toe

A small educational project: a game of Tic-Tac-Toe built on a custom, simple HTTP server based on Boost.Asio.

## Build requirements

- CMake 3.28 or newer
- a C++23 compiler
- Boost 1.82 or newer with Boost.Log
- hiredis
- OpenSSL
- Ninja

The remaining C++ dependencies are downloaded at pinned revisions by CMake.
Redis is required at runtime, but not to compile the server.

## Build

```shell
cmake --preset debug
cmake --build --preset debug --parallel
```

The executable is written to `build/debug/Tic_Tac_Toe_Boost`.

For an optimized build:

```shell
cmake --preset release
cmake --build --preset release --parallel
```
