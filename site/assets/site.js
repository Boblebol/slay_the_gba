// The English pages (site/en/) say so on <html lang="en">: counts below follow it.
const english = document.documentElement.lang === "en";

// Card filters (type / rarity / text) and simple list searches.
document.querySelectorAll(".filters:not([data-progress])").forEach((box) => {
  const scope = box.parentElement;
  const cards = () => scope.querySelectorAll(".card");
  const state = { type: "", rarity: "", text: "" };
  const count = box.querySelector(".count");

  const apply = () => {
    let shown = 0;
    cards().forEach((card) => {
      const ok =
        (!state.type || card.dataset.type === state.type) &&
        (!state.rarity || card.dataset.rarity === state.rarity) &&
        (!state.text || card.dataset.name.includes(state.text));
      card.classList.toggle("hidden", !ok);
      if (ok) shown += 1;
    });
    if (count) count.textContent = `${shown} ${english ? "card" : "carte"}${shown > 1 ? "s" : ""}`;
  };

  box.querySelectorAll("[data-filter-type]").forEach((button) => {
    button.addEventListener("click", () => {
      state.type = button.dataset.filterType;
      box.querySelectorAll("[data-filter-type]").forEach((b) => b.classList.toggle("on", b === button));
      apply();
    });
  });
  box.querySelectorAll("[data-filter-rarity]").forEach((button) => {
    button.addEventListener("click", () => {
      state.rarity = button.dataset.filterRarity;
      box.querySelectorAll("[data-filter-rarity]").forEach((b) => b.classList.toggle("on", b === button));
      apply();
    });
  });
  const search = box.querySelector("[data-search]");
  if (search) {
    search.addEventListener("input", () => {
      state.text = search.value.trim().toLowerCase();
      apply();
    });
  }
  apply();
});

document.querySelectorAll("[data-solo]").forEach((input) => {
  const selector = input.dataset.solo;
  input.addEventListener("input", () => {
    const text = input.value.trim().toLowerCase();
    document.querySelectorAll(selector).forEach((item) => {
      item.classList.toggle("hidden", !!text && !item.textContent.toLowerCase().includes(text));
    });
  });
});

// Progress page: filter the checklist by status and text, folding the categories left empty.
document.querySelectorAll("[data-progress]").forEach((box) => {
  const state = { status: "", text: "" };
  const count = box.querySelector(".count");

  const apply = () => {
    const filtering = !!(state.status || state.text);
    let shown = 0;
    document.querySelectorAll("details.prog-cat").forEach((category) => {
      let visible = 0;
      category.querySelectorAll("li").forEach((item) => {
        const ok =
          (!state.status || item.dataset.status === state.status) &&
          (!state.text || item.dataset.name.includes(state.text));
        item.classList.toggle("hidden", !ok);
        if (ok) visible += 1;
      });
      category.classList.toggle("hidden", visible === 0);
      // A text search opens the matching categories; a status filter alone would unfold a thousand rows.
      if (filtering) category.open = !!state.text && visible > 0;
      shown += visible;
    });
    document.querySelectorAll(".prog-section").forEach((section) => {
      section.classList.toggle("hidden", !section.querySelector("details.prog-cat:not(.hidden)"));
    });
    if (count) count.textContent = `${shown} ${english ? "item" : "élément"}${shown > 1 ? "s" : ""}`;
  };

  box.querySelectorAll("[data-filter-status]").forEach((button) => {
    button.addEventListener("click", () => {
      state.status = button.dataset.filterStatus;
      box.querySelectorAll("[data-filter-status]").forEach((b) => b.classList.toggle("on", b === button));
      apply();
    });
  });
  box.querySelector("[data-search]").addEventListener("input", (event) => {
    state.text = event.target.value.trim().toLowerCase();
    apply();
  });
  apply();
});

// Landing page: the console plays captures of the ROM in a loop, like a cartridge's attract mode.
const consoleShot = document.getElementById("console-shot");
const still = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
let attract = 0;
if (consoleShot && !still) {
  const shots = consoleShot.dataset.cycle.split(",");
  shots.forEach((src) => { new Image().src = src; });
  let shown = 0;
  attract = setInterval(() => {
    shown = (shown + 1) % shots.length;
    consoleShot.src = shots[shown];
  }, 2600);
}

// Landing page: edition picker (hero sprites and cartridge tabs) and screenshot gallery.
const editionButtons = document.querySelectorAll("[data-edition]");
if (editionButtons.length) {
  const select = (id) => {
    document.body.dataset.edition = id;
    document.body.className = [...document.body.classList].filter((c) => !c.startsWith("hero-")).concat(`hero-${id}`).join(" ");
    editionButtons.forEach((b) => b.setAttribute("aria-pressed", String(b.dataset.edition === id)));
    document.querySelectorAll("[data-panel]").forEach((p) => { p.hidden = p.dataset.panel !== id; });
    // The console of the title screen stops its demo loop and shows the picked hero's battle.
    const pick = document.querySelector(`.pick[data-edition="${id}"]`);
    if (consoleShot && pick) {
      clearInterval(attract);
      consoleShot.src = pick.dataset.shot;
    }
  };
  editionButtons.forEach((button) => button.addEventListener("click", () => select(button.dataset.edition)));
}

const galleryMain = document.getElementById("gallery-main");
if (galleryMain) {
  const caption = document.getElementById("gallery-caption");
  document.querySelectorAll(".thumb").forEach((thumb) => {
    thumb.addEventListener("click", (event) => {
      event.preventDefault();
      galleryMain.src = thumb.getAttribute("href");
      galleryMain.alt = caption.textContent = thumb.dataset.caption;
      document.querySelectorAll(".thumb").forEach((t) => t.classList.toggle("on", t === thumb));
    });
  });
}

// START menu (phones): a modal window opened from the dock, closed by its button, Escape or a tap outside.
const startMenu = document.getElementById("start-menu");
if (startMenu && typeof startMenu.showModal === "function") {
  const openers = document.querySelectorAll("[data-menu]");
  const setExpanded = (open) => openers.forEach((b) => b.setAttribute("aria-expanded", String(open)));
  openers.forEach((b) => b.addEventListener("click", () => { startMenu.showModal(); setExpanded(true); }));
  startMenu.addEventListener("close", () => setExpanded(false));
  startMenu.querySelector("[data-close]").addEventListener("click", () => startMenu.close());
  startMenu.addEventListener("click", (event) => {
    if (event.target === startMenu || event.target.closest("a")) startMenu.close();
  });
}

// Compendium drop-down (wide screens): one open at a time, closed by a click elsewhere or Escape.
const dropdowns = document.querySelectorAll(".dd details");
document.addEventListener("click", (event) => {
  dropdowns.forEach((d) => { if (!d.contains(event.target)) d.open = false; });
});
document.addEventListener("keydown", (event) => {
  if (event.key === "Escape") dropdowns.forEach((d) => { if (d.open) { d.open = false; d.querySelector("summary").focus(); } });
});

// "Readable text" option: the system font for paragraphs, remembered in this browser only.
const readableButtons = document.querySelectorAll("[data-readable]");
const showReadable = () => {
  const on = document.documentElement.classList.contains("readable");
  readableButtons.forEach((b) => b.setAttribute("aria-pressed", String(on)));
};
readableButtons.forEach((button) => button.addEventListener("click", () => {
  const on = document.documentElement.classList.toggle("readable");
  try { localStorage.setItem("readable", on ? "1" : "0"); } catch (e) { /* private mode: the option lasts for the page */ }
  showReadable();
}));
showReadable();
