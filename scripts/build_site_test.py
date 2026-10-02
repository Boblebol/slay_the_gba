"""Tests of the website generator's formatting helpers, on small hand-written inputs.

Nothing is written to the repository: every output goes to a tmp_path.
Run from the repository root:  uv run --python 3.12 --with pytest --with markdown --with pillow pytest scripts/build_site_test.py
"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

if sys.version_info < (3, 12):
    # build_site.py declares requires-python >= 3.12 (it nests quotes inside f-strings) and does not even compile before.
    pytest.skip("build_site.py needs Python 3.12 or newer", allow_module_level=True)

from PIL import Image

import build_site
import progress
from build_site import (STATUS_BADGE, card_html, cards_by_id, download_button, esc, img_path, md_to_html, meter, rich,
                        rom_size, stars, starter_html)
from editions import HEROES


@pytest.fixture(autouse=True)
def isolated_site(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    """Point every output folder of the generator at tmp_path and start each test with an empty sitemap."""
    site = tmp_path / "site"
    (site / "assets" / "screens").mkdir(parents=True)
    (site / "roms").mkdir()
    monkeypatch.setattr(build_site, "SITE", site)
    monkeypatch.setattr(build_site, "ASSETS", site / "assets")
    monkeypatch.setattr(build_site, "ROMS", site / "roms")
    monkeypatch.setattr(build_site, "PAGES", [])
    return site


# --- HTML escaping ---------------------------------------------------------------------------


def test_esc_escapes_markup_and_both_kinds_of_quote() -> None:
    assert esc('<a href="x">&\'') == "&lt;a href=&quot;x&quot;&gt;&amp;&#x27;"


def test_esc_accepts_numbers_and_keeps_accents() -> None:
    assert esc(42) == "42"
    assert esc("Évènement « Boss »") == "Évènement « Boss »"


def test_rich_turns_the_dump_markup_into_html() -> None:
    assert rich("<g>Block</g>") == '<b class="kw">Block</b>'
    assert rich("<s>2</s>") == '2 <span class="star">★</span>'
    assert rich("<e>1</e>") == '1 <span class="nrg">●</span>'
    assert rich("Gain [E]") == 'Gain <span class="nrg">●</span>'
    assert rich("<e/>") == '<span class="nrg">●</span>'
    assert rich("a\nb") == "a<br>b"


def test_rich_drops_unknown_tags_and_escapes_the_rest() -> None:
    assert rich("<b>x</b> a&b") == "x a&amp;b"
    assert rich('"quoted"') == "&quot;quoted&quot;"


def test_rich_of_nothing_is_empty() -> None:
    assert rich("") == ""


# --- Links and paths -------------------------------------------------------------------------


def test_img_path_climbs_one_level_more_than_the_page_depth() -> None:
    assert img_path("/images/cards/x.webp", 0) == "../bible/images/cards/x.webp"
    assert img_path("/images/cards/x.webp", 2) == "../../../bible/images/cards/x.webp"


def test_rom_size_is_rounded_to_kibibytes_or_asks_for_a_build(tmp_path: Path) -> None:
    hero = HEROES[0]
    assert rom_size(hero) == "à compiler"
    (build_site.ROMS / hero.rom).write_bytes(b"\0" * 2048)
    assert rom_size(hero) == "2 Ko"


def test_download_button_links_to_the_rom_relative_to_the_page() -> None:
    hero = HEROES[1]
    button = download_button(hero, 2)
    assert f'href="../../roms/{hero.rom}"' in button and " download>" in button
    assert "(à compiler)" in button
    assert 'class="btn"' in button and 'class="big"' in download_button(hero, 0, "big")


def test_md_to_html_rewrites_tome_and_image_links() -> None:
    text = "![a](images/x.png)\n\n[tome](04_foo.md#sec) [booklet](livrets_gba/livret_silent.md) [web](https://e.com/a.md)"
    out = md_to_html(text, 1, "sub/")
    assert 'src="../../bible/sub/images/x.png"' in out and 'loading="lazy"' in out
    assert 'href="04_foo.html#sec"' in out
    assert 'href="livret_silent.html"' in out
    assert 'href="https://e.com/a.md"' in out  # external links are left alone


def test_md_to_html_shows_source_links_as_code() -> None:
    out = md_to_html("[card.h](../include/engine/card.h)", 0, "")
    assert "<code>card.h</code>" in out and "<a" not in out


# --- Small fragments -------------------------------------------------------------------------


def test_stars_fill_to_three_with_the_difficulty_label() -> None:
    assert stars(1) == '<span class="diff" title="Accessible">★☆☆</span>'
    assert stars(3) == '<span class="diff" title="Expert">★★★</span>'


def test_cards_by_id_and_starter_deck_lines() -> None:
    cards = [{"id": "BASH", "name": "Heurt"}, {"id": "STRIKE_IRONCLAD", "name": "Frappe <b>"}]
    by_id = cards_by_id(cards)
    assert by_id["BASH"] is cards[0]
    ironclad = HEROES[0]
    deck = {"STRIKE_IRONCLAD": "Frappe", "DEFEND_IRONCLAD": "Défense", "BASH": "Heurt"}
    assert starter_html(ironclad, {k: {"name": v} for k, v in deck.items()}) == "5× Frappe, 4× Défense, 1× Heurt"
    assert starter_html(ironclad, {k: {"name": "<i>"} for k in deck}).startswith("5× &lt;i&gt;")


def test_card_html_shows_cost_and_only_an_upgrade_that_differs() -> None:
    card = {"id": "BASH", "name": "Heurt", "type": "Attack", "rarity": "Basic", "pool": "Ironclad", "energy": 2,
            "imagePath": "/images/cards/ironclad/bash.webp", "descBase": "Deal <g>8</g> damage.",
            "descUpgraded": "Deal <g>10</g> damage.", "keywords": ["Vulnerable"]}
    out = card_html(card, 1)
    assert 'class="card t-attack p-ironclad"' in out
    assert '<span class="gem">2</span>' in out
    assert "Attaque · Base · Vulnerable" in out
    assert "Amélioré +" in out and "10" in out
    same = {**card, "descUpgraded": card["descBase"]}
    assert "Amélioré +" not in card_html(same, 1)


@pytest.mark.parametrize("energy", [-1, None])
def test_card_html_shows_x_for_variable_cost(energy: int | None) -> None:
    card = {"id": "WHIRLWIND", "name": "Tourbillon", "type": "Attack", "rarity": "Uncommon", "pool": "Ironclad",
            "energy": energy, "imagePath": "/x.webp", "descBase": "d"}
    assert '<span class="gem">X</span>' in card_html(card, 0)


def test_card_html_escapes_the_name() -> None:
    card = {"id": "A", "name": '<script>"', "type": "Skill", "rarity": "Rare", "pool": "Event", "energy": 0,
            "imagePath": "/x.webp", "descBase": "d"}
    out = card_html(card, 0)
    assert "<script>" not in out and "&lt;script&gt;&quot;" in out


# --- Percentages and progress ----------------------------------------------------------------


def test_meter_of_an_empty_tally_is_zero_wide() -> None:
    out = meter(progress.Tally(), "Rien")
    assert 'aria-valuenow="0"' in out and '<span style="width:0">' in out


def test_meter_keeps_a_visible_sliver_for_tiny_percentages() -> None:
    out = meter(progress.Tally(done=1, todo=999), "Peu")  # 0.1 %
    assert "width:max(3px, 0.1%)" in out and 'aria-valuenow="0"' in out


def test_meter_rounds_the_percentage_and_escapes_the_label() -> None:
    out = meter(progress.Tally(done=1, todo=2), 'A "b"', "#c82030")  # 33.3 %
    assert 'aria-valuenow="33"' in out and "width:max(3px, 33.3%)" in out
    assert 'aria-label="A &quot;b&quot;"' in out and 'style="--fill:#c82030"' in out


def test_every_status_has_a_badge() -> None:
    assert set(STATUS_BADGE) == {progress.DONE, progress.PARTIAL, progress.TODO, progress.EXCLUDED}


def test_progress_item_links_the_issue_and_escapes_text() -> None:
    item = progress.Item("A_B", "Carte <x>", progress.PARTIAL, detail="d", note="n", issue=12)
    out = build_site.progress_item(item)
    assert f'href="{progress.ISSUE_URL}12"' in out and ">#12</a>" in out
    assert "Carte &lt;x&gt;" in out and f'data-status="{progress.PARTIAL}"' in out
    assert 'data-name="carte &lt;x&gt; a_b"' in out  # searchable by name and by key


def test_progress_item_without_note_or_issue_has_no_small_tag() -> None:
    out = build_site.progress_item(progress.Item("BASH", "BASH", progress.DONE))
    assert "<small>" not in out and 'data-name="bash"' in out


def test_progress_category_shows_counts_and_a_french_percentage() -> None:
    category = progress.Category("Cartes", (progress.Item("a", "a", progress.DONE), progress.Item("b", "b", progress.TODO)))
    out = build_site.progress_category(category)
    assert "50,0 %" in out and "1 fait, 0 partiel, 1 à faire" in out and "<details" in out
    assert "<details" not in build_site.progress_category(category, items=False)


# --- Sorting and generated files -------------------------------------------------------------


def test_build_list_page_orders_groups_by_label_then_items_by_name(isolated_site: Path) -> None:
    items = [
        {"name": "Zèbre", "pool": "Ironclad", "imagePath": "/z.webp", "descI18n": "z"},
        {"name": "Abeille", "pool": "Ironclad", "imagePath": "/a.webp", "descI18n": "a"},
        {"name": "Mystère", "pool": "Inconnu", "imagePath": "/m.webp", "descI18n": "m"},
        {"name": "Commune", "pool": "Shared", "imagePath": "/c.webp", "descI18n": "c"},
    ]
    labels = {"Shared": "Partagées", "Ironclad": "Soldat de Fer"}
    build_site.build_list_page("reliques.html", "Reliques", items, "pool", labels, "descI18n", "reliques.html")
    out = (isolated_site / "reliques.html").read_text()
    assert out.index("Partagées") < out.index("Soldat de Fer") < out.index("Inconnu")  # unlabelled groups come last
    assert out.index("Abeille") < out.index("Zèbre")
    assert "Soldat de Fer <small>(2)</small>" in out
    assert "4 entrées" in out


def test_page_wraps_a_body_and_collects_indexable_paths() -> None:
    out = build_site.page("Cartes <1>", "<p>corps</p>", depth=1, active="cartes.html", path="cartes.html")
    assert "<title>Cartes &lt;1&gt; — Slay the GBA</title>" in out and "<p>corps</p>" in out
    assert f'<link rel="canonical" href="{build_site.SITE_URL}/cartes.html">' in out
    assert 'href="../assets/style.css"' in out
    assert 'content="index, follow"' in out
    assert build_site.PAGES == ["cartes.html"]


def test_page_marked_noindex_stays_out_of_the_sitemap_list() -> None:
    out = build_site.page("Mentions", "", path="mentions.html", noindex=True)
    assert 'content="noindex, follow"' in out and build_site.PAGES == []
    assert 'href="/assets/style.css"' in build_site.page("x", "", depth=3, absolute=True)


def test_build_seo_files_dedupes_pages_and_ranks_them(isolated_site: Path) -> None:
    Image.new("RGB", (4, 4)).save(isolated_site / "assets" / "screens" / "title.png")
    build_site.PAGES.extend(["", "heros/silent.html", "cartes.html", "cartes.html"])
    build_site.build_seo_files()
    sitemap = (isolated_site / "sitemap.xml").read_text()
    base = build_site.SITE_URL
    assert sitemap.count("<url>") == 3
    assert f"<loc>{base}/</loc><priority>1.0</priority>" in sitemap
    assert f"<loc>{base}/heros/silent.html</loc><priority>0.8</priority>" in sitemap
    assert f"<loc>{base}/cartes.html</loc><priority>0.6</priority>" in sitemap
    robots = (isolated_site / "robots.txt").read_text()
    assert "Disallow: /roms/" in robots and f"Sitemap: {base}/sitemap.xml" in robots
    with Image.open(isolated_site / "assets" / "og.png") as og:
        assert og.size == (1200, 630)
