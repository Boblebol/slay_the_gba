// Looming Fruit: raises Max HP by 31 once, when obtained.
#include <cassert>
#include <iostream>
#include "engine/run.h"

using namespace engine;

int main() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 2024);
    int16_t max_hp = run.max_hp;
    // Without the relic, Max HP is untouched.
    assert(!run.has_relic("LOOMING_FRUIT"));
    run.add_relic("LOOMING_FRUIT");
    assert(run.max_hp == max_hp + 31);
    std::cout << "[PASS] Looming Fruit: +31 Max HP on pickup" << std::endl;
    return 0;
}
