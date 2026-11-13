COMPOSE := docker compose

.PHONY: configure build release sanitize run tests clean \
	infra-up infra-down infra-reset infra-restart up down restart start

configure:
	cmake --preset debug

build: configure
	cmake --build --preset debug --parallel

release:
	cmake --preset release
	cmake --build --preset release --parallel

sanitize:
	cmake --preset asan-ubsan
	cmake --build --preset asan-ubsan --parallel

run: build
	./build/debug/Tic_Tac_Toe_Boost

clean:
	cmake --build --preset debug --target clean

infra-up:
	$(COMPOSE) up --detach

infra-down:
	$(COMPOSE) down

infra-reset:
	$(COMPOSE) down --volumes

infra-restart: infra-down infra-up

# Backward-compatible aliases.
up: infra-up
down: infra-down
restart: infra-restart
start: infra-up run

tests:
	./test_runner.sh
