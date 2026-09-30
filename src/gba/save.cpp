#include "gba/save.h"

#include "bn_span.h"
#include "bn_sram.h"

#include "engine/save_system.h"

namespace gba
{

namespace
{

class SramStorage final : public engine::SaveStorage
{
public:
    void read(void* destination, size_t size, size_t offset) override
    {
        bn::span<uint8_t> bytes(static_cast<uint8_t*>(destination), int(size));
        bn::sram::read_span_offset(bytes, int(offset));
    }

    void write(const void* source, size_t size, size_t offset) override
    {
        bn::span<const uint8_t> bytes(static_cast<const uint8_t*>(source), int(size));
        bn::sram::write_span_offset(bytes, int(offset));
    }
};

SramStorage sram_storage;

static_assert(2 * engine::SAVE_BANK_SIZE <= bn::sram::size(), "Save banks do not fit in SRAM");

}

void init_save()
{
    engine::SaveManager& save = engine::SaveManager::instance();
    save.set_storage(&sram_storage);
    save.load_from_storage();
}

}
