// Card filters (type / rarity / text) and simple list searches.
document.querySelectorAll(".filters").forEach((box) => {
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
