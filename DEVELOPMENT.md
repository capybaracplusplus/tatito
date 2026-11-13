# Development

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
