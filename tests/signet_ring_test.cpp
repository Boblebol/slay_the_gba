// Signet Ring: 888 gold when obtained, nothing without it.
#include <cassert>
#include <iostream>
#include "engine/run.h"

using namespace engine;

namespace {

void test_signet_ring() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    int16_t before = run.gold;
    run.add_relic("SIGNET_RING");
    assert(run.has_relic("SIGNET_RING"));
    assert(run.gold == before + 888);

    RunManager other;
    other.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    int16_t start = other.gold;
    other.add_relic("STRAWBERRY");
    assert(other.gold == start);
}

}  // namespace

int main() {
    std::cout << "=== Signet Ring ===" << std::endl;
    test_signet_ring();
    std::cout << "All tests passed" << std::endl;
    return 0;
}
