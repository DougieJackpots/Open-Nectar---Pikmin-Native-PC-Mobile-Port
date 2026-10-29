// Shared call sites keep their interface; the multiplayer implementation and
// alternate captain model lookups are omitted from single-player builds.
#include "pc_coop.h"
#if !PIKI_SINGLE_PLAYER_ONLY
#error "This implementation requires PIKI_SINGLE_PLAYER_ONLY"
#endif

static bool sChosenAtTitle = false;
void pc_coop_set_pending(bool) {}
void pc_coop_set_chosen_at_title(bool on) { sChosenAtTitle = on; }
bool pc_coop_take_chosen_at_title()
{
    const bool chosen = sChosenAtTitle;
    sChosenAtTitle = false;
    return chosen;
}
void pc_coop_set_captain(int, int) {}
int pc_coop_captain(int) { return PC_CAPTAIN_OLIMAR; }
int pc_captain_piki_color(int) { return -1; }
bool pc_captain_is_pikmin(int) { return false; }
int pc_captain_skeleton_color(int) { return -1; }
int pc_captain_pikmin2_model(int) { return -1; }
bool pc_captain_available(int captain) { return captain == PC_CAPTAIN_OLIMAR; }
bool pc_coop_p2_tinted() { return false; }
void pc_coop_p2_tint(unsigned char* r, unsigned char* g, unsigned char* b)
{
    if (r) *r = 255;
    if (g) *g = 255;
    if (b) *b = 255;
}
void pc_vs_set_pending(bool) {}
void pc_coop_begin_run() {}
void pc_coop_end_run() {}
