#include <repositories/game_repository.h>

namespace repositories::game {
GameRepository::GameRepository(
    std::shared_ptr<sw::redis::Redis> dbClient) noexcept
    : dbClient_(std::move(dbClient)) {}

std::optional<std::string> GameRepository::find(const std::string &) noexcept {
  return std::optional<std::string>();
}

void GameRepository::add(const MatchmakingSesion &) noexcept {}

bool GameRepository::remove(void) { return false; }

} // namespace repositories::game
