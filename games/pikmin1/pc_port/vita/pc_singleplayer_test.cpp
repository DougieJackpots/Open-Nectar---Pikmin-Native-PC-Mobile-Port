#include "pc_coop.h"
#include <cstdio>
int main()
{
    pc_coop_set_pending(true);
    pc_vs_set_pending(true);
    pc_coop_set_captain(0, PC_CAPTAIN_LOUIE);
    pc_coop_set_captain(1, PC_CAPTAIN_PIKMIN_RED);
    pc_coop_begin_run();
    if (pc_coop_pending() || pc_coop_active() || pc_vs_pending() || pc_vs_active()) return 1;
    if (pc_coop_captain(0) != PC_CAPTAIN_OLIMAR || pc_coop_captain(1) != PC_CAPTAIN_OLIMAR) return 1;
    for (int i = -1; i <= PC_CAPTAIN_COUNT; ++i)
        if (pc_captain_available(i) != (i == PC_CAPTAIN_OLIMAR)) return 1;
    unsigned char r = 0, g = 0, b = 0;
    pc_coop_p2_tint(&r, &g, &b);
    if (pc_coop_p2_tinted() || r != 255 || g != 255 || b != 255) return 1;
    pc_coop_set_chosen_at_title(true);
    if (!pc_coop_take_chosen_at_title() || pc_coop_take_chosen_at_title()) return 1;
    pc_coop_end_run();
    std::puts("Single-player feature gate: PASS");
    return 0;
}
