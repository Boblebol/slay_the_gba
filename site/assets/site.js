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
    if (count) count.textContent = `${shown} carte${shown > 1 ? "s" : ""}`;
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
    if (count) count.textContent = `${shown} élément${shown > 1 ? "s" : ""}`;
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

// Landing page: edition picker (hero sprites and cartridge tabs) and screenshot gallery.
const editionButtons = document.querySelectorAll("[data-edition]");
if (editionButtons.length) {
  const select = (id) => {
    document.body.dataset.edition = id;
    document.body.className = [...document.body.classList].filter((c) => !c.startsWith("hero-")).concat(`hero-${id}`).join(" ");
    editionButtons.forEach((b) => b.setAttribute("aria-pressed", String(b.dataset.edition === id)));
    document.querySelectorAll("[data-panel]").forEach((p) => { p.hidden = p.dataset.panel !== id; });
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
