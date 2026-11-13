# Tic-tac-toe backend on Boost.Asio

An experimental backend for tic-tac-toe built with C++23, Boost.Asio,
Redis, and a small HTTP server.

## Project history

The initial prototype was written in 2024-2025 as an experiment with
Boost.Asio, a custom HTTP server, and Redis. Development resumed in 2026.
The original architecture and Git history are being preserved while the
networking, concurrency, persistence, tests, and game logic are completed.

The original unfinished state is available at the `prototype-2025` tag.

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

## Sanitized build

AppleClang, Clang, and GCC builds can enable AddressSanitizer and
UndefinedBehaviorSanitizer together:

```shell
cmake --preset asan-ubsan
cmake --build --preset asan-ubsan --parallel
ASAN_OPTIONS=halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
./build/asan-ubsan/Tic_Tac_Toe_Boost
```

## Runtime infrastructure

Start the two Redis instances used by the current prototype:

```shell
docker compose up --detach
```

Or use the Makefile wrappers:

```shell
make build
make sanitize
make infra-up
make run
```

`make infra-reset` removes the Redis containers and their persisted volumes.

## Tests

The current Python tests are integration tests and expect both Redis and the
server to be running:

```shell
make infra-up
make run
# In another terminal:
make tests
```
