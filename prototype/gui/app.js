
(() => {
  "use strict";

  const app = document.getElementById("app");
  const $ = (s) => app.querySelector(s);
  const $$ = (s) => [...app.querySelectorAll(s)];

  let toastTimer = null;
  const say = (text) => {
    $("#statusText").innerHTML = `<i class="status-dot"></i>${text}`;
    const toast = $("#toast");
    toast.textContent = text;
    toast.classList.add("show");
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => toast.classList.remove("show"), 1500);
  };

  const openDialog = (title, message, details = "") =>
    new Promise((resolve) => {
      $("#dialogTitle").textContent = title;
      $("#dialogMessage").textContent = message;
      $("#dialogDetails").innerHTML = details;
      $("#dialogBackdrop").style.display = "flex";

      const close = (value) => {
        $("#dialogBackdrop").style.display = "none";
        $("#dialogOk").onclick = null;
        $("#dialogCancel").onclick = null;
        resolve(value);
      };

      $("#dialogOk").onclick = () => close(true);
      $("#dialogCancel").onclick = () => close(false);
    });

  const library = [
    ["Heaven", "ADD + PCM", "1.8K"],
    ["Glass Pad", "ADD", "2.1K"],
    ["Motion 1", "ADD", "2.8K"],
    ["Digital Rain", "ADD + PCM", "1.6K"],
    ["Warm Air", "PCM", "1.2K"],
    ["Sphaera", "ADD", "3.4K"],
    ["Wire Choir", "ADD + PCM", "2.3K"],
    ["Dark Bell", "ADD", "1.7K"],
    ["Vector Ice", "PCM", "1.1K"],
    ["Slow Orbit", "ADD", "2.5K"],
    ["Metal Breath", "ADD + PCM", "2.0K"],
    ["Night Glass", "ADD", "1.9K"]
  ];

  const seedNames = [
    "Warm Air", "Glass Pad", "Runner", "Heaven", "Bell Matrix", "Digi Sweep",
    "Slow Orbit", "Choirglass", "Night Motion", "Metal Breath", "Vector Ice",
    "Sphaera", "Dark Bell", "Space Choir", "Airframe", "Tiny Bell", "Orbit Lead",
    "Wide Pad", "Glass Organ", "Hybrid Vox", "Wire Choir", "Dark Drone",
    "Digital Rain", "Soft Metal", "Frozen Key", "Night Glass", "Motion 1",
    "Blue Formant", "Crystal Vox", "Deep Motion", "Skyline", "Voice Dust"
  ];

  const makeBank = (type, size) =>
    Array.from({ length: size }, (_, i) => {
      const limit = type === "A" ? 58 : type === "D" ? 42 : 28;
      if (i >= limit) return "";
      const offset = type === "D" ? 7 : 0;
      const base = seedNames[(i + offset) % seedNames.length];
      return i >= seedNames.length ? `${base} ${Math.floor(i / seedNames.length) + 1}` : base;
    });

  let banks = {
    A: makeBank("A", 128),
    D: makeBank("D", 128),
    M: makeBank("M", 64)
  };

  let currentBank = "A";
  let visibleCount = 64;
  let selected = new Set();
  let anchor = null;
  let clipboard = null;
  let undoStack = [];
  let redoStack = [];

  const maxSlots = () => currentBank === "M" ? 64 : 128;
  const slotId = (i) => `${currentBank === "M" ? "M" : currentBank}${String(i + 1).padStart(3, "0")}`;

  const snapshot = () => ({
    A: [...banks.A],
    D: [...banks.D],
    M: [...banks.M]
  });

  const restore = (state) => {
    banks = {
      A: [...state.A],
      D: [...state.D],
      M: [...state.M]
    };
    renderSlots();
  };

  const pushUndo = (label) => {
    undoStack.push({ label, state: snapshot() });
    redoStack = [];
    updateUndoButtons();
  };

  const updateUndoButtons = () => {
    $("#undoBtn").disabled = undoStack.length === 0;
    $("#redoBtn").disabled = redoStack.length === 0;
  };

  function renderLibrary() {
    const query = $("#searchInput").value.toLowerCase();
    const filter = $(".chip.active")?.dataset.filter || "all";
    const host = $("#libraryList");
    host.innerHTML = "";

    library
      .filter(([name, src]) => {
        const nameOk = name.toLowerCase().includes(query);
        const filterOk =
          filter === "all" ||
          (filter === "add" && src.includes("ADD")) ||
          (filter === "pcm" && src === "PCM") ||
          (filter === "mixed" && src.includes("+"));
        return nameOk && filterOk;
      })
      .forEach((entry) => {
        const [name, src, size] = entry;
        const row = document.createElement("button");
        row.type = "button";
        row.className = "library-row";
        row.draggable = true;
        row.innerHTML = `
          <div>
            <div class="library-name">${name}</div>
            <div class="library-meta">Library · ${size}</div>
          </div>
          <span class="source-label">${src}</span>
        `;
        row.addEventListener("dragstart", (e) => {
          e.dataTransfer.setData("application/x-k5000-preset", JSON.stringify(entry));
        });
        row.addEventListener("click", () => say(`Library: ${name}`));
        host.appendChild(row);
      });
  }

  function renderSlots() {
    const grid = $("#slotGrid");
    const count = Math.min(visibleCount, maxSlots());
    grid.className = `slot-grid view-${visibleCount}`;
    grid.innerHTML = "";

    selected = new Set([...selected].filter((i) => i < count));

    for (let i = 0; i < count; i++) {
      const name = banks[currentBank][i];
      const slot = document.createElement("button");
      slot.type = "button";
      slot.className = [
        "slot",
        name ? "" : "empty",
        selected.has(i) ? "selected" : "",
        clipboard?.cut && clipboard.bank === currentBank && clipboard.indices.includes(i) ? "cut" : ""
      ].filter(Boolean).join(" ");

      const source = currentBank === "M"
        ? "Multi"
        : (i % 3 === 0 ? "ADD + PCM" : i % 3 === 1 ? "ADD" : "PCM");

      slot.innerHTML = `
        <div class="slot-num">${slotId(i)}</div>
        <div class="slot-name">${name || "Empty"}</div>
        <div class="slot-meta">${name ? `${source} · ${(1.1 + (i % 5) * .35).toFixed(1)}K` : "Drop preset here"}</div>
      `;

      slot.addEventListener("click", (e) => selectSlot(i, e));
      slot.addEventListener("contextmenu", (e) => openContextMenu(i, e));
      slot.addEventListener("dragover", (e) => e.preventDefault());
      slot.addEventListener("drop", (e) => dropPreset(i, e));

      grid.appendChild(slot);
    }

    updateSummary();
    updateInspector();
  }

  function refreshSelectionClasses() {
    [...$("#slotGrid").children].forEach((node, i) => {
      node.classList.toggle("selected", selected.has(i));
    });
    updateSummary();
    updateInspector();
  }

  function selectSlot(i, event) {
    if (event.ctrlKey) {
      selected.has(i) ? selected.delete(i) : selected.add(i);
      anchor = i;
    } else if (event.shiftKey && anchor !== null) {
      selected.clear();
      for (let x = Math.min(anchor, i); x <= Math.max(anchor, i); x++) selected.add(x);
    } else {
      selected = new Set([i]);
      anchor = i;
    }
    refreshSelectionClasses();
  }

  function updateSummary() {
    const bankName = currentBank === "M" ? "Multi" : `Bank ${currentBank}`;
    $("#bankTitle").textContent = bankName;
    $("#occupiedText").textContent = `${banks[currentBank].filter(Boolean).length} / ${maxSlots()} occupied`;
    $("#capacityText").textContent =
      currentBank === "M" ? "64 Multi positions" :
      currentBank === "A" ? "ADD 121 / 137" : "ADD 84 / 137";
    $("#selectionText").textContent = `${selected.size} selected`;
    $("#meterFill").style.width = currentBank === "A" ? "88%" : currentBank === "D" ? "61%" : "44%";

    $("#convertBankSyx").textContent =
      currentBank === "M" ? "Convert Multi Bank to SysEx" : `Convert Bank ${currentBank} to SysEx`;
    $("#sendBankImg").textContent =
      currentBank === "M" ? "Send Multi Bank to .IMG" : `Send Bank ${currentBank} to .IMG`;
    $("#exportSelectedTop").textContent =
      currentBank === "M" ? "Export Multi(s) as KC1" : "Export Patch(es) as KA1";
    $("#exportSelectedBtn").textContent =
      currentBank === "M" ? "Export as KC1" : "Export as KA1";
  }

  function updateInspector() {
    const indices = [...selected].sort((a, b) => a - b);

    if (indices.length === 0) {
      $("#inspectorSlot").textContent = "—";
      $("#inspectorName").textContent = "No selection";
      $("#inspectorOrigin").textContent = "Select one or more slots";
      $("#inspectorSources").textContent = "—";
      $("#inspectorSize").textContent = "—";
      $("#inspectorCount").textContent = "0 items";
      return;
    }

    const i = indices[0];
    const name = banks[currentBank][i] || "Empty";

    $("#inspectorSlot").textContent = indices.length === 1 ? slotId(i) : `${indices.length} slots`;
    $("#inspectorName").textContent = indices.length === 1 ? name : "Multiple selection";
    $("#inspectorOrigin").textContent = currentBank === "M" ? "Multi workspace" : `Bank ${currentBank} workspace`;
    $("#inspectorSources").textContent =
      currentBank === "M" ? "Multi" : (i % 3 === 0 ? "ADD + PCM" : i % 3 === 1 ? "ADD" : "PCM");
    $("#inspectorSize").textContent =
      indices.length === 1 && banks[currentBank][i]
        ? `${(1.1 + (i % 5) * .35).toFixed(1)}K`
        : "—";
    $("#inspectorCount").textContent = `${indices.length} item${indices.length === 1 ? "" : "s"}`;
  }

  function dropPreset(i, e) {
    e.preventDefault();
    const raw = e.dataTransfer.getData("application/x-k5000-preset");
    if (!raw) return;
    if (currentBank === "M") {
      say("Single presets cannot be dropped into Multi workspace");
      return;
    }
    const [name] = JSON.parse(raw);
    pushUndo("Drop preset");
    banks[currentBank][i] = name;
    selected = new Set([i]);
    anchor = i;
    renderSlots();
    say(`${name} → ${slotId(i)}`);
  }

  function copySelection(cut) {
    if (!selected.size) {
      say("Select slot(s) first");
      return;
    }
    const indices = [...selected].sort((a, b) => a - b);
    clipboard = {
      bank: currentBank,
      indices,
      values: indices.map((i) => banks[currentBank][i]),
      cut
    };
    renderSlots();
    say(`${cut ? "Cut" : "Copied"} ${indices.length} item(s)`);
  }

  function pasteSelection() {
    if (!clipboard) return say("Clipboard empty");
    if (!selected.size) return say("Select destination slot");

    if ((currentBank === "M") !== (clipboard.bank === "M")) {
      say("Single and Multi clipboard types are incompatible");
      return;
    }

    const dest = Math.min(...selected);
    pushUndo("Paste");

    clipboard.values.forEach((value, offset) => {
      if (dest + offset < banks[currentBank].length) {
        banks[currentBank][dest + offset] = value;
      }
    });

    if (clipboard.cut) {
      clipboard.indices.forEach((i) => {
        banks[clipboard.bank][i] = "";
      });
      clipboard = null;
    }

    selected = new Set([dest]);
    renderSlots();
    say("Paste completed");
  }

  function clearSelection() {
    if (!selected.size) return say("Select slot(s) first");
    const count = selected.size;
    pushUndo("Clear");
    selected.forEach((i) => banks[currentBank][i] = "");
    renderSlots();
    say(`Cleared ${count} item(s)`);
  }

  function openContextMenu(i, event) {
    event.preventDefault();

    if (!selected.has(i)) selected = new Set([i]);
    refreshSelectionClasses();

    const menu = $("#contextMenu");
    $("#contextTitle").textContent =
      `${selected.size} selected ${currentBank === "M" ? "Multi preset(s)" : "patch(es)"}`;

    const exportButton = menu.querySelector('[data-action="export"]');
    exportButton.textContent =
      currentBank === "M" ? "Export Multi(s) as KC1…" : "Export Patch(es) as KA1…";

    menu.style.display = "block";

    const appRect = app.getBoundingClientRect();
    const left = Math.min(event.clientX - appRect.left, appRect.width - 260);
    const top = event.clientY - appRect.top;
    menu.style.left = `${Math.max(0, left)}px`;
    menu.style.top = `${Math.max(0, top)}px`;
  }

  async function handleAction(action) {
    if (action === "copy") return copySelection(false);
    if (action === "cut") return copySelection(true);
    if (action === "paste") return pasteSelection();
    if (action === "clear") return clearSelection();

    if (action === "rename") {
      say(selected.size === 1 ? "Rename workflow simulated" : "Select exactly one item to rename");
      return;
    }

    if (!selected.size) return say("Select item(s) first");

    if (action === "export") {
      const ext = currentBank === "M" ? "KC1" : "KA1";
      const examples = [...selected]
        .slice(0, 5)
        .map((i) => `${slotId(i)}_${banks[currentBank][i] || "Empty"}.${ext}`)
        .join("<br>");
      const ok = await openDialog(
        `Export ${selected.size} item(s) as ${ext}`,
        `Each selected slot will be exported as an individual ${ext} file.`,
        examples || "No occupied slots selected"
      );
      if (ok) say(`${ext} export simulated`);
    }

    if (action === "syx") {
      const ok = await openDialog(
        `Convert ${selected.size} item(s) to SysEx`,
        `Selected ${currentBank === "M" ? "Multis" : "patches"} will become individual .syx files.`,
        `Source: ${currentBank === "M" ? "Multi" : `Bank ${currentBank}`}<br>Selection: ${selected.size} item(s)`
      );
      if (ok) say("SysEx conversion simulated");
    }

    if (action === "img") {
      const ok = await openDialog(
        `Send ${selected.size} item(s) to .IMG`,
        "Selected items will be serialized and added to the active disk-image workspace.",
        `Target: 023_MODERN.IMG<br>Mode: individual ${currentBank === "M" ? "KC1" : "KA1"} files`
      );
      if (ok) say("Selection sent to IMG workspace (simulation)");
    }
  }

  $("#contextMenu").addEventListener("click", (e) => {
    const action = e.target.dataset.action;
    if (!action) return;
    $("#contextMenu").style.display = "none";
    handleAction(action);
  });

  app.addEventListener("click", (e) => {
    if (!e.target.closest("#contextMenu")) $("#contextMenu").style.display = "none";
  });

  $$("#bankSwitch button").forEach((button) => {
    button.addEventListener("click", () => {
      currentBank = button.dataset.bank;
      selected.clear();
      anchor = null;

      $$("#bankSwitch button").forEach((x) => x.classList.toggle("active", x === button));

      if (currentBank === "M" && visibleCount === 128) visibleCount = 64;

      $$("#visibleSwitch button").forEach((x) => {
        x.disabled = currentBank === "M" && x.dataset.count === "128";
        x.classList.toggle("active", Number(x.dataset.count) === visibleCount);
      });

      renderSlots();
      say(currentBank === "M" ? "Multi workspace" : `Bank ${currentBank}`);
    });
  });

  $$("#visibleSwitch button").forEach((button) => {
    button.addEventListener("click", () => {
      visibleCount = Number(button.dataset.count);
      $$("#visibleSwitch button").forEach((x) => x.classList.toggle("active", x === button));
      renderSlots();
      say(`${visibleCount} slots visible`);
    });
  });

  // Lasso selection
  const canvas = $("#bankCanvas");
  const lasso = $("#lasso");
  let lassoStart = null;

  canvas.addEventListener("pointerdown", (e) => {
    if (e.button !== 0 || e.target.closest(".slot")) return;

    const rect = canvas.getBoundingClientRect();
    lassoStart = {
      x: e.clientX - rect.left + canvas.scrollLeft,
      y: e.clientY - rect.top + canvas.scrollTop
    };

    lasso.style.display = "block";
    lasso.style.left = `${lassoStart.x}px`;
    lasso.style.top = `${lassoStart.y}px`;
    lasso.style.width = "0px";
    lasso.style.height = "0px";
    canvas.setPointerCapture(e.pointerId);
  });

  canvas.addEventListener("pointermove", (e) => {
    if (!lassoStart) return;
    const rect = canvas.getBoundingClientRect();
    const x = e.clientX - rect.left + canvas.scrollLeft;
    const y = e.clientY - rect.top + canvas.scrollTop;

    lasso.style.left = `${Math.min(lassoStart.x, x)}px`;
    lasso.style.top = `${Math.min(lassoStart.y, y)}px`;
    lasso.style.width = `${Math.abs(x - lassoStart.x)}px`;
    lasso.style.height = `${Math.abs(y - lassoStart.y)}px`;
  });

  canvas.addEventListener("pointerup", () => {
    if (!lassoStart) return;

    const lassoRect = lasso.getBoundingClientRect();
    selected.clear();

    [...$("#slotGrid").children].forEach((node, i) => {
      const rect = node.getBoundingClientRect();
      const intersects = !(
        rect.right < lassoRect.left ||
        rect.left > lassoRect.right ||
        rect.bottom < lassoRect.top ||
        rect.top > lassoRect.bottom
      );
      if (intersects) selected.add(i);
    });

    lasso.style.display = "none";
    lassoStart = null;
    refreshSelectionClasses();
    say(`Lasso selected ${selected.size} item(s)`);
  });

  // Top/current-bank actions
  $("#exportSelectedTop").addEventListener("click", () => handleAction("export"));
  $("#exportSelectedBtn").addEventListener("click", () => handleAction("export"));
  $("#convertSelectedBtn").addEventListener("click", () => handleAction("syx"));
  $("#sendSelectedImgBtn").addEventListener("click", () => handleAction("img"));

  $("#copyBtn").addEventListener("click", () => copySelection(false));
  $("#cutBtn").addEventListener("click", () => copySelection(true));
  $("#pasteBtn").addEventListener("click", pasteSelection);
  $("#clearBtn").addEventListener("click", clearSelection);
  $("#renameBtn").addEventListener("click", () => handleAction("rename"));

  $("#convertBankSyx").addEventListener("click", async () => {
    const title = currentBank === "M" ? "Convert Multi Bank to SysEx" : `Convert Bank ${currentBank} to SysEx`;
    const ok = await openDialog(
      title,
      "Convert the complete active bank using the verified K5000 SysEx representation.",
      `Occupied positions: ${banks[currentBank].filter(Boolean).length}`
    );
    if (ok) say("Whole-bank SysEx conversion simulated");
  });

  let imageFiles = ["MODERN_A.KAA", "MODERN_D.KAA", "LIVE.KCA", "ARPEGGIO.KRA"];

  function renderImageFiles() {
    const host = $("#imgFileList");
    host.innerHTML = "";
    imageFiles.forEach((name, i) => {
      const row = document.createElement("button");
      row.type = "button";
      row.className = "image-file";
      row.innerHTML = `
        <span class="muted">${String(i + 1).padStart(2, "0")}</span>
        <strong>${name}</strong>
        <span class="muted">${name.endsWith(".KAA") ? "Single Bank" : name.endsWith(".KCA") ? "Multi Bank" : "File"}</span>
      `;
      row.addEventListener("click", () => {
        $$(".image-file").forEach((x) => x.classList.remove("selected"));
        row.classList.add("selected");
      });
      host.appendChild(row);
    });
  }

  $("#sendBankImg").addEventListener("click", async () => {
    const bankLabel = currentBank === "M" ? "Multi Bank" : `Bank ${currentBank}`;
    const containerName = currentBank === "M" ? "LIVE.KCA" : `BANK_${currentBank}.KAA`;
    const ok = await openDialog(
      `Send ${bankLabel} to .IMG`,
      "Serialize the complete current bank and add it to the active disk image.",
      `Target: 023_MODERN.IMG<br>Container: ${containerName}`
    );
    if (ok) {
      if (!imageFiles.includes(containerName)) imageFiles.push(containerName);
      renderImageFiles();
      say(`${bankLabel} sent to IMG workspace (simulation)`);
    }
  });

  $("#createImgBtn").addEventListener("click", () => say("Create IMG from Workspace simulated"));
  $("#imgAddBtn").addEventListener("click", () => say("Add Files simulated"));
  $("#extractAllBtn").addEventListener("click", () => say("Extract All simulated"));
  $("#deepExtractBtn").addEventListener("click", () => say("Deep Extract simulated"));
  $("#validateImgBtn").addEventListener("click", () => say("Validate Image simulated"));

  $("#imgRemoveBtn").addEventListener("click", () => {
    const selectedRow = $(".image-file.selected");
    if (!selectedRow) return say("Select an image file first");
    const name = selectedRow.querySelector("strong").textContent;
    imageFiles = imageFiles.filter((x) => x !== name);
    renderImageFiles();
    say(`Removed ${name}`);
  });

  const dropZone = $("#imgDropZone");
  ["dragenter", "dragover"].forEach((type) => {
    dropZone.addEventListener(type, (e) => {
      e.preventDefault();
      dropZone.classList.add("drag");
    });
  });
  ["dragleave", "drop"].forEach((type) => {
    dropZone.addEventListener(type, (e) => {
      e.preventDefault();
      dropZone.classList.remove("drag");
    });
  });
  dropZone.addEventListener("drop", (e) => {
    const names = [...e.dataTransfer.files].map((f) => f.name).filter(Boolean);
    imageFiles.push(...names.slice(0, 10));
    renderImageFiles();
    say(`${names.length} Explorer file(s) added`);
  });

  $("#syxSelectionBtn").addEventListener("click", () => handleAction("syx"));
  $("#syxBankBtn").addEventListener("click", () => $("#convertBankSyx").click());

  $("#undoBtn").addEventListener("click", () => {
    if (!undoStack.length) return;
    const item = undoStack.pop();
    redoStack.push({ label: item.label, state: snapshot() });
    restore(item.state);
    updateUndoButtons();
    say(`Undo: ${item.label}`);
  });

  $("#redoBtn").addEventListener("click", () => {
    if (!redoStack.length) return;
    const item = redoStack.pop();
    undoStack.push({ label: item.label, state: snapshot() });
    restore(item.state);
    updateUndoButtons();
    say(`Redo: ${item.label}`);
  });

  $("#validateBtn").addEventListener("click", () => say("Workspace validation simulated"));
  $("#saveBtn").addEventListener("click", () => say("Save simulated"));

  $("#searchInput").addEventListener("input", renderLibrary);
  $$(".chip").forEach((chip) => {
    chip.addEventListener("click", () => {
      $$(".chip").forEach((x) => x.classList.remove("active"));
      chip.classList.add("active");
      renderLibrary();
    });
  });

  $$(".tab").forEach((tab) => {
    tab.addEventListener("click", () => {
      $$(".tab").forEach((x) => x.classList.remove("active"));
      tab.classList.add("active");
      $$(".page").forEach((x) => x.classList.remove("active"));
      $(`#page-${tab.dataset.page}`).classList.add("active");
      say(`${tab.textContent} view`);
    });
  });

  // Classic Windows shortcuts
  app.addEventListener("keydown", (e) => {
    if (e.target.matches("input, textarea, select")) return;
    const key = e.key.toLowerCase();

    if (e.ctrlKey && key === "c") { e.preventDefault(); copySelection(false); }
    else if (e.ctrlKey && key === "x") { e.preventDefault(); copySelection(true); }
    else if (e.ctrlKey && key === "v") { e.preventDefault(); pasteSelection(); }
    else if (e.ctrlKey && key === "z" && !e.shiftKey) { e.preventDefault(); $("#undoBtn").click(); }
    else if ((e.ctrlKey && key === "y") || (e.ctrlKey && e.shiftKey && key === "z")) { e.preventDefault(); $("#redoBtn").click(); }
    else if (e.ctrlKey && key === "a") {
      e.preventDefault();
      selected = new Set(Array.from({ length: Math.min(visibleCount, maxSlots()) }, (_, i) => i));
      refreshSelectionClasses();
    }
    else if (e.ctrlKey && key === "f") { e.preventDefault(); $("#searchInput").focus(); }
    else if (e.key === "Delete") { e.preventDefault(); clearSelection(); }
    else if (e.key === "F2") { e.preventDefault(); handleAction("rename"); }
    else if (e.key === "Escape") {
      selected.clear();
      $("#contextMenu").style.display = "none";
      refreshSelectionClasses();
      say("Selection cleared");
    }
  });

  renderLibrary();
  renderSlots();
  renderImageFiles();
  updateUndoButtons();
  app.focus();
})();
