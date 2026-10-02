"""Tests of the cartridge memory check.

Run from the repository root:  uv run --with pytest pytest scripts/check_rom_memory_test.py
"""

from __future__ import annotations

import pytest

import check_rom_memory

MAP = """
                0x03007fa0                        __sp_irq = (__iwram_top - 0x60)
                0x03007f00                        __sp_usr = (__sp_irq - 0xa0)
                0x03002bd8                        __bss_end__ = ABSOLUTE (.)
                0x03002c00                        __iheap_start = .
"""


def test_stack_room_is_the_gap_between_static_data_and_the_stack_top() -> None:
    assert check_rom_memory.stack_room(MAP) == 0x03007F00 - 0x03002C00


def test_a_card_table_in_ram_leaves_too_little_room() -> None:
    # Layout of the v0.39.0 cartridges, which hung at boot: the card table was built in IWRAM at start-up.
    crowded = MAP.replace("0x03002c00", "0x03007774")
    assert check_rom_memory.stack_room(crowded) < check_rom_memory.MIN_STACK_ROOM


def test_a_missing_symbol_is_an_error() -> None:
    with pytest.raises(SystemExit):
        check_rom_memory.symbol(MAP, "__nowhere")
