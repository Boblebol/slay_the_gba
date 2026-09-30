#pragma once

namespace gba
{

// Installs the cartridge battery SRAM as the save storage and loads the save it holds.
// Must be called once after bn::core::init().
void init_save();

}
