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
    monkeypatch.setattr(build_site, "LANG", "fr")
    return site


@pytest.fixture
def english(monkeypatch: pytest.MonkeyPatch) -> None:
    """Build the English pages (site/en/)."""
    monkeypatch.setattr(build_site, "LANG", "en")


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


# --- Landing page ----------------------------------------------------------------------------


def make_progress() -> progress.Progress:
    def item(name: str, status: str, issue: int | None = None) -> progress.Item:
        return progress.Item(name, name, status, issue=issue)

    sections = [progress.Section(h.slug, h.name, (progress.Category("Mécaniques", (
        item("Force", progress.DONE), item("Poison", progress.PARTIAL), item("Orbes", progress.DONE))),), h)
        for h in HEROES]
    sections += [progress.Section(slug, slug.title(), (progress.Category("Système", (
        item(f"{slug}-fait", progress.DONE), item(f"{slug}-cours", progress.PARTIAL, 7),
        item(f"{slug}-todo", progress.TODO, 8))),)) for slug in ("systemes", "interface", "regles", "cartes")]
    return progress.Progress(tuple(sections))


@pytest.mark.parametrize("percent, color", [(100, "#68e868"), (85, "#68e868"), (84.9, "#f8d048"), (55, "#f8d048"),
                                            (25, "#f0a060"), (24.9, "#f85848"), (0, "#f85848")])
def test_progress_color_thresholds(percent: float, color: str) -> None:
    assert build_site.progress_color(percent) == color


def test_hero_mechanics_lists_only_done_items_up_to_the_limit() -> None:
    section = make_progress().section("ironclad")
    assert build_site.hero_mechanics(section) == ["Force", "Orbes"]
    assert build_site.hero_mechanics(section, limit=1) == ["Force"]


def test_journal_items_keep_priority_sections_and_skip_catalogues() -> None:
    prog = make_progress()
    assert [i.name for i in build_site.journal_items(prog, progress.PARTIAL)] == [
        "systemes-cours", "interface-cours", "regles-cours"]  # "cartes" is an auto catalogue, left out


def test_journal_items_are_capped(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(build_site, "JOURNAL_LIMIT", 2)
    assert len(build_site.journal_items(make_progress(), progress.DONE)) == 2


def test_landing_journal_links_issues_and_colours_columns() -> None:
    out = build_site.landing_journal(make_progress())
    assert f'href="{progress.ISSUE_URL}7"' in out and ">#8</a>" in out
    assert "<h3>Fait</h3>" in out and "<h3>En cours</h3>" in out and "<h3>À venir</h3>" in out
    assert "--jc:#68e868" in out and "--jc:#f8d048" in out and "--jc:#9a98b0" in out


def test_landing_progress_has_one_bar_per_section_and_a_detail_link() -> None:
    prog = make_progress()
    out = build_site.landing_progress(prog)
    assert out.count('class="sect"') == len(prog.sections)
    assert progress.percent_fr(prog.percent) in out and progress.GLOBAL_RULE in out and 'href="avancement.html"' in out
    assert "cnt-done" in out and "cnt-todo" in out


def test_launch_sprites_and_tabs_cover_every_edition_with_one_pressed() -> None:
    for out in (build_site.launch_sprites(), "".join(build_site.edition_tab(h) for h in HEROES)):
        assert [m for m in out.split('data-edition="')[1:]] and out.count("data-edition=") == len(HEROES)
        assert out.count('aria-pressed="true"') == 1


def test_landing_captures_use_plain_links_so_it_works_without_js() -> None:
    out = build_site.landing_captures()
    assert out.count('class="thumb"') == len(build_site.SCREENS)
    assert 'id="gallery-main"' in out and 'href="assets/screens/title.png"' in out


def test_build_index_renders_five_panels_and_keeps_seo_and_nav(isolated_site: Path) -> None:
    cards = [{"id": cid, "pool": "Ironclad", "name": cid.title()} for h in HEROES for cid, _ in h.starter]
    relics = [{"id": h.relic_id, "name": f"Relique {h.slug}"} for h in HEROES]
    build_site.build_index(cards, [{"acts": [1]}], relics, [], [], make_progress())
    out = (isolated_site / "index.html").read_text()
    assert out.count("data-panel=") == len(HEROES)
    assert out.count('" hidden>') == len(HEROES) - 1  # only the default edition is visible without JS
    assert 'class="hero-ironclad"' in out and 'property="og:title"' in out and 'class="ae-footer"' in out
    for anchor in ("editions", "avancement", "fait", "captures", "bible", "telecharger", "faq"):
        assert f'id="{anchor}"' in out
    assert "<details class='faq'>" in out and "Choisissez votre héros" in out
    assert f'<link rel="canonical" href="{build_site.SITE_URL}/">' in out


def test_page_switches_language_and_links_its_translation() -> None:
    fr = build_site.page("Accueil", "", path="")
    assert '<html lang="fr">' in fr and 'href="en/index.html" hreflang="en"' in fr and 'hreflang="x-default"' in fr
    cards = build_site.page("Cartes", "", path="cartes.html")
    assert 'href="en/cartes.html" hreflang="en"' in cards and 'hreflang="x-default"' not in cards
    assert f'hreflang="en" href="{build_site.SITE_URL}/en/cartes.html"' in cards
    assert f'hreflang="fr" href="{build_site.SITE_URL}/cartes.html"' in cards


def test_english_page_lives_under_en_and_links_back_to_french(english: None) -> None:
    out = build_site.page("Silent", "", depth=1, path="heros/silent.html")
    assert '<html lang="en">' in out and 'content="en_US"' in out
    assert 'href="../../heros/silent.html" hreflang="fr"' in out  # site/en/heros/ -> site/heros/
    assert f'<link rel="canonical" href="{build_site.SITE_URL}/en/heros/silent.html">' in out
    assert 'href="../../assets/style.css"' in out and 'href="../cartes.html"' in out  # assets are shared
    assert 'href="../../bible/index.html">Bible (FR)' in out
    assert "unofficial fan game" in out and build_site.ARTICLE_URL_EN in out
    assert build_site.PAGES == ["en/heros/silent.html"]


def test_untranslated_page_switches_to_the_other_landing() -> None:
    out = build_site.page("Tome", "", depth=1, path="bible/01_x.html", translated=False)
    assert 'href="../en/index.html" hreflang="en"' in out and 'rel="alternate"' not in out


def test_english_pages_are_written_under_en(isolated_site: Path, english: None) -> None:
    build_site.write_page("heros/x.html", "x")
    assert (isolated_site / "en" / "heros" / "x.html").read_text() == "x"


@pytest.mark.parametrize(("lang", "text"), [("fr", "testeurs recherchés"), ("en", "playtesters wanted")])
def test_testers_panel_links_feedback_and_celebrates_completion(lang: str, text: str,
                                                                monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(build_site, "LANG", lang)
    prog = make_progress()
    monkeypatch.setattr(type(prog), "percent", property(lambda self: 100.0))
    out = build_site.testers_panel(prog)
    assert text in out and "100" in out and build_site.FEEDBACK_URL in out and 'class="panel callout"' in out


def test_english_landing_says_the_game_is_french_and_offers_every_rom(isolated_site: Path, english: None) -> None:
    cards = [{"id": cid, "pool": "Ironclad", "name": cid.title()} for h in HEROES for cid, _ in h.starter]
    relics = [{"id": h.relic_id, "name": f"Relique {h.slug}"} for h in HEROES]
    build_site.build_index(cards, [{"acts": [1]}], relics, [], [], make_progress())
    out = (isolated_site / "en" / "index.html").read_text()
    assert '<html lang="en">' in out and "The game is in French" in out and "Choose your hero" in out
    for h in HEROES:
        assert f'href="../roms/{h.rom}"' in out and h.english in out
    for anchor in ("language", "testers", "editions", "telecharger", "faq"):
        assert f'id="{anchor}"' in out
    assert 'src="../assets/sprites/hero_ironclad.png"' in out
    assert build_site.PAGES == ["en/index.html"]


def test_english_card_uses_the_english_template_and_class_name(english: None) -> None:
    card = {"id": "BASH", "className": "Bash", "name": "Heurt", "type": "Attack", "rarity": "Basic",
            "pool": "Ironclad", "energy": 2, "imagePath": "/x.webp", "description": "Deal {Damage:diff()} damage.",
            "vars": {"Damage": 8}, "upgrade": {"damage": "+2"}, "descBase": "Infligez 8 dégâts.",
            "descUpgraded": "Infligez 10 dégâts."}
    out = card_html(card, 0)
    assert "<h4>Bash</h4>" in out and "Attack · Basic" in out and "Deal 8 damage." in out
    assert "Upgraded +" in out and "Deal 10 damage." in out and 'src="../../bible/x.webp"' in out


def test_english_card_without_a_usable_template_falls_back_to_french(english: None) -> None:
    card = {"id": "X", "className": "MadScience", "name": "Science folle", "type": "Skill", "rarity": "Rare",
            "pool": "Event", "imagePath": "/x.webp", "description": "{CardType:choose(Attack):a|b}",
            "descBase": "Texte <g>français</g>."}
    assert 'Texte <b class="kw">français</b>.' in card_html(card, 0) and "<h4>Mad Science</h4>" in card_html(card, 0)


def test_english_tracker_items_use_data_names_and_translated_texts(english: None,
                                                                   monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(build_site, "EN_NAMES", {"STRIKE_IRONCLAD": "Strike"})
    data_item = progress.Item("STRIKE_IRONCLAD", "Frappe", progress.DONE, "Base · Attaque")
    assert build_site.item_name(data_item) == "Strike" and build_site.item_detail(data_item) == "Basic · Attack"
    assert build_site.item_name(progress.Item("BATTLEWORN_DUMMY", "Mannequin", progress.DONE)) == "Battleworn Dummy"
    out = build_site.progress_item(progress.Item("Mécaniques", "Mécaniques", progress.PARTIAL))
    assert "<b>Mechanics</b>" in out and "Partial" in out
    assert build_site.counts(progress.Tally(3, 1, 2, 1)) == "3 done, 1 partial, 2 to do (1 out of scope)"
    assert build_site.percent(12.34) == "12.3%"


def test_sitemap_ranks_english_pages_like_their_french_counterparts(isolated_site: Path) -> None:
    Image.new("RGB", (4, 4)).save(isolated_site / "assets" / "screens" / "title.png")
    build_site.PAGES.extend(["en/index.html", "en/heros/silent.html", "en/cartes.html"])
    build_site.build_seo_files()
    sitemap = (isolated_site / "sitemap.xml").read_text()
    base = build_site.SITE_URL
    assert f"<loc>{base}/en/index.html</loc><priority>1.0</priority>" in sitemap
    assert f"<loc>{base}/en/heros/silent.html</loc><priority>0.8</priority>" in sitemap
    assert f"<loc>{base}/en/cartes.html</loc><priority>0.6</priority>" in sitemap


def test_page_shell_has_compendium_menu_dock_and_local_font() -> None:
    out = build_site.page("Cartes", "", path="cartes.html", active="cartes.html")
    assert "fonts.googleapis.com" not in out and 'href="assets/fonts/slay-gba.woff2"' in out
    assert '<li class="dd on"><details><summary>Compendium</summary>' in out  # the current page's group is lit
    assert out.count('href="cartes.html" class="on" aria-current="page"') == 2  # header drop-down and START menu
    assert 'id="start-menu"' in out and out.count("data-menu") == 1 and 'class="dock"' in out
    assert '<span class="lang on" aria-current="true">FR</span>' in out and "data-readable" in out


def test_console_cycles_real_captures() -> None:
    out = build_site.console()
    assert 'id="console-shot"' in out and 'src="assets/screens/title.png"' in out
    assert out.count("assets/screens/") == len(build_site.ATTRACT_SCREENS) + 1
