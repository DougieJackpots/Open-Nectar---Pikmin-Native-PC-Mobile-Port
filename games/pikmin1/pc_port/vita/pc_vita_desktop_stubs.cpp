// Desktop integrations are not available on the handheld. Local speedrun
// timing still works; these hooks neither open sockets nor spawn threads.
#include "pc_discord.h"
#include "pc_livesplit.h"
void pc_discord_init(const char*) {}
void pc_discord_shutdown() {}
void pc_discord_set_playing(const char*, const char*, const char*) {}
void pc_livesplit_update(bool, bool, uint64_t) {}
bool pc_livesplit_connected() { return false; }
void pc_livesplit_start() {}
void pc_livesplit_split(uint64_t) {}
void pc_livesplit_reset() {}
