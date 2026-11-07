/* FieldScene Inspector — vanilla JS viewer */
"use strict";

const S = {
  data: null,
  controls: [],
  byId: new Map(),
  refsByValue: new Map(),
  images: new Map(),
  patterns: new Map(),
  selected: null,
  focus: new Set(),
  hover: null,
  filter: "all",
  query: "",
  tab: "controls",
  labels: false,
  outlines: true,
  view: { scale: 1, x: 0, y: 0 },
  dragging: false,
  dragMoved: false,
  dragStart: { x: 0, y: 0, vx: 0, vy: 0 },
  world: { w: 800, h: 600 },
  dpr: 1,
};

const STATUS_COLORS = {
  resolved: "#50c878",
  orphan: "#4696ff",
  unnamed: "#ffaa28",
  mismatch: "#f04646",
};

const $ = (id) => document.getElementById(id);
const canvas = $("scene");
const ctx = canvas.getContext("2d");

function esc(value) {
  return String(value == null ? "" : value)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}

function argb(v) {
  v = (Number(v) >>> 0);
  return "rgba(" + ((v >> 16) & 255) + "," + ((v >> 8) & 255) + "," + (v & 255) + "," + ((v >>> 24) & 255) / 255 + ")";
}

function badge(status) {
  return '<span class="badge ' + esc(status) + '">' + esc(status) + "</span>";
}

function controlName(c) {
  return c.primary_name || c.proposed_name || "";
}

// ---------------------------------------------------------------------------
// boot
// ---------------------------------------------------------------------------
fetch("/api/state")
  .then((r) => {
    if (!r.ok) throw new Error("HTTP " + r.status);
    return r.json();
  })
  .then((data) => {
    S.data = data;
    S.controls = data.controls;
    S.controls.forEach((c) => S.byId.set(c.id, c));
    for (const ref of data.references || []) {
      if (ref.value == null) continue;
      if (!S.refsByValue.has(ref.value)) S.refsByValue.set(ref.value, []);
      S.refsByValue.get(ref.value).push(ref);
    }
    const bounds = data.meta.bounds || [0, 0, 800, 600];
    S.world.w = Math.max(800, bounds[2]);
    S.world.h = Math.max(600, bounds[3]);
    $("meta").textContent =
      data.meta.control_count + " controls · " + data.meta.unique_ids + " ids · " +
      data.meta.summary.dead_values + " dead refs · " +
      data.meta.summary.orphan_controls + " unbound · " +
      data.meta.summary.unnamed_controls + " unnamed";
    renderList();
    renderDead();
    renderProblems();
    resize();
    fit();
    applyHash();
    $("loading").classList.add("hidden");
  })
  .catch((err) => {
    $("loading").innerHTML = '<div><strong>Failed to load</strong><div class="small">' + esc(err.message) + "</div></div>";
  });

// ---------------------------------------------------------------------------
// canvas / camera
// ---------------------------------------------------------------------------
function resize() {
  const rect = canvas.parentElement.getBoundingClientRect();
  S.dpr = window.devicePixelRatio || 1;
  canvas.width = Math.max(1, Math.floor(rect.width * S.dpr));
  canvas.height = Math.max(1, Math.floor(rect.height * S.dpr));
  draw();
}

function fit() {
  const rect = canvas.parentElement.getBoundingClientRect();
  const scale = Math.min(rect.width / S.world.w, rect.height / S.world.h) * 0.96;
  S.view.scale = Math.max(0.05, scale);
  S.view.x = (rect.width - S.world.w * S.view.scale) / 2;
  S.view.y = (rect.height - S.world.h * S.view.scale) / 2;
  updateViewInfo();
  draw();
}

function updateViewInfo() {
  $("viewInfo").textContent = "zoom " + Math.round(S.view.scale * 100) + "%";
}

function screenToWorld(x, y) {
  return { x: (x - S.view.x) / S.view.scale, y: (y - S.view.y) / S.view.scale };
}

function checkerPattern() {
  const c = document.createElement("canvas");
  c.width = c.height = 16;
  const g = c.getContext("2d");
  g.fillStyle = "#1b1e24";
  g.fillRect(0, 0, 16, 16);
  g.fillStyle = "#22262e";
  g.fillRect(0, 0, 8, 8);
  g.fillRect(8, 8, 8, 8);
  return ctx.createPattern(c, "repeat");
}

let CHECKER = null;

function draw() {
  if (!S.data) return;
  const w = canvas.width / S.dpr;
  const h = canvas.height / S.dpr;
  ctx.setTransform(S.dpr, 0, 0, S.dpr, 0, 0);
  ctx.clearRect(0, 0, w, h);
  ctx.fillStyle = "#0f1115";
  ctx.fillRect(0, 0, w, h);

  ctx.save();
  ctx.translate(S.view.x, S.view.y);
  ctx.scale(S.view.scale, S.view.scale);

  if (!CHECKER) CHECKER = checkerPattern();
  ctx.fillStyle = CHECKER;
  ctx.fillRect(0, 0, S.world.w, S.world.h);
  ctx.strokeStyle = "rgba(120,140,170,.35)";
  ctx.lineWidth = 1 / S.view.scale;
  ctx.strokeRect(0, 0, S.world.w, S.world.h);

  const visible = filteredSet();
  const allow = S.filter !== "all" || S.query.trim() !== "";
  ctx.imageSmoothingEnabled = false;
  for (const c of S.controls) {
    const matches = !allow || visible.has(c.id);
    drawControl(c, matches ? 1 : 0.10, matches);
  }
  ctx.restore();
  updateViewInfo();
}

function drawControl(c, alpha, matches) {
  const [x, y, w, h] = c.rect;
  const tex = c.type === "3DOBJ" ? -1 : c.tex;
  ctx.globalAlpha = alpha;

  if (tex === -2) {
    ctx.fillStyle = argb(c.fields.color == null ? 0 : c.fields.color);
    ctx.fillRect(x, y, w, h);
  } else if (tex >= 0 || tex < -2) {
    const sprite = c.sprite;
    const img = sprite ? getImage(sprite.url) : null;
    if (img) {
      drawSprite(c, img, sprite);
    } else if (matches && tex >= 0) {
      ctx.strokeStyle = "rgba(240,70,70,.75)";
      ctx.lineWidth = 1 / S.view.scale;
      ctx.strokeRect(x + 0.5, y + 0.5, w, h);
    }
  }

  if (c.type === "TEXT" || (c.type === "BUTTON" && c.text) || c.type === "EDIT") {
    const value = c.type === "EDIT" ? (c.fields.value || "") : (c.text || "");
    if (value) {
      ctx.fillStyle = argb(c.fields.font_color == null ? 0xFFFFFFFF : c.fields.font_color);
      ctx.font = "12px Tahoma, Verdana, sans-serif";
      ctx.textBaseline = "top";
      ctx.save();
      ctx.beginPath();
      ctx.rect(x, y, Math.max(1, w), Math.max(1, h));
      ctx.clip();
      ctx.fillText(value, x + 2, y + Math.max(1, (h - 12) / 2));
      ctx.restore();
    }
  }

  if (c.type === "PROGRESS") {
    const current = c.fields.current || 0;
    const max = c.fields.max_value || 0;
    if (max > 0) {
      const frac = Math.max(0, Math.min(1, current / max));
      ctx.fillStyle = argb(c.fields.progress_color == null ? 0xFF00FF00 : c.fields.progress_color);
      ctx.fillRect(x + 2, y + 2, Math.max(0, (w - 4) * frac), Math.max(1, h - 4));
    }
  }

  if (c.type === "3DOBJ") {
    ctx.fillStyle = "rgba(150,160,180,.10)";
    ctx.fillRect(x, y, w, h);
    ctx.fillStyle = "rgba(210,220,240,.7)";
    ctx.font = "10px Consolas, monospace";
    ctx.fillText("3D", x + 3, y + 2);
  }

  if (S.outlines && matches) {
    const color = STATUS_COLORS[c.status] || "#78808f";
    ctx.strokeStyle = color;
    ctx.lineWidth = 1 / S.view.scale;
    ctx.strokeRect(x + 0.5 / S.view.scale, y + 0.5 / S.view.scale, w, h);
  }
  if (S.focus.has(c.id)) {
    ctx.strokeStyle = "#36e0e0";
    ctx.lineWidth = 2 / S.view.scale;
    ctx.setLineDash([4 / S.view.scale, 3 / S.view.scale]);
    ctx.strokeRect(x, y, w, h);
    ctx.setLineDash([]);
  }
  if (S.hover && S.hover.id === c.id) {
    ctx.strokeStyle = "rgba(255,255,255,.85)";
    ctx.lineWidth = 2 / S.view.scale;
    ctx.strokeRect(x, y, w, h);
  }
  if (S.selected && S.selected.id === c.id) {
    ctx.strokeStyle = "#ffeb3c";
    ctx.lineWidth = 3 / S.view.scale;
    ctx.strokeRect(x - 1, y - 1, w + 2, h + 2);
  }

  if (S.labels && matches && S.view.scale > 0.35) {
    const label = "#" + c.id + (controlName(c) ? " " + controlName(c) : "");
    ctx.font = (10 / S.view.scale) + "px Consolas, monospace";
    const tw = ctx.measureText(label).width;
    ctx.fillStyle = "rgba(0,0,0,.72)";
    ctx.fillRect(x, y, tw + 4 / S.view.scale, 12 / S.view.scale);
    ctx.fillStyle = "#e8eefb";
    ctx.fillText(label, x + 2 / S.view.scale, y + 1 / S.view.scale);
  }
  ctx.globalAlpha = 1;
}

function drawSprite(c, img, sprite) {
  const [x, y, w, h] = c.rect;
  const [dx, dy] = sprite.dest || [0, 0];
  if (c.type === "PANEL" || c.type === "LISTBOX") {
    const fill = c.fields.fill_type;
    if (fill === 2) {
      ctx.drawImage(img, x + dx, y + dy);
    } else if (fill === 3) {
      tileImage(img, x, y, w, h);
    } else {
      ctx.drawImage(img, x + dx, y + dy, Math.max(1, w), Math.max(1, h));
    }
    return;
  }
  if (c.type === "GRID") {
    const rows = Math.max(1, c.fields.rows || 1);
    const cols = Math.max(1, c.fields.columns || 1);
    const cw = w / cols;
    const ch = h / rows;
    for (let r = 0; r < rows; r++) {
      for (let k = 0; k < cols; k++) {
        ctx.drawImage(img, x + k * cw, y + r * ch, Math.max(1, cw), Math.max(1, ch));
      }
    }
    return;
  }
  ctx.drawImage(img, x + dx, y + dy, Math.max(1, w), Math.max(1, h));
}

function tileImage(img, x, y, w, h) {
  const key = img.src + "|" + w + "x" + h;
  let pat = S.patterns.get(key);
  if (!pat) {
    pat = ctx.createPattern(img, "repeat");
    S.patterns.set(key, pat);
  }
  if (!pat) return;
  ctx.save();
  ctx.translate(x, y);
  ctx.fillStyle = pat;
  ctx.fillRect(0, 0, w, h);
  ctx.restore();
}

function getImage(url) {
  if (!url) return null;
  if (S.images.has(url)) {
    const img = S.images.get(url);
    return img && img.complete && img.naturalWidth ? img : null;
  }
  const img = new Image();
  img.onload = () => draw();
  img.onerror = () => draw();
  img.src = url;
  S.images.set(url, img);
  return null;
}

// ---------------------------------------------------------------------------
// hit testing / interaction
// ---------------------------------------------------------------------------
function hitTest(wx, wy) {
  for (let i = S.controls.length - 1; i >= 0; i--) {
    const c = S.controls[i];
    const [x, y, w, h] = c.rect;
    if (w <= 0 || h <= 0) continue;
    if (wx >= x && wy >= y && wx <= x + w && wy <= y + h) return c;
  }
  return null;
}

canvas.addEventListener("wheel", (e) => {
  e.preventDefault();
  const rect = canvas.getBoundingClientRect();
  const mx = e.clientX - rect.left;
  const my = e.clientY - rect.top;
  const factor = e.deltaY < 0 ? 1.15 : 1 / 1.15;
  const next = Math.max(0.05, Math.min(8, S.view.scale * factor));
  const k = next / S.view.scale;
  S.view.x = mx - (mx - S.view.x) * k;
  S.view.y = my - (my - S.view.y) * k;
  S.view.scale = next;
  draw();
}, { passive: false });

canvas.addEventListener("contextmenu", (e) => e.preventDefault());

canvas.addEventListener("mousedown", (e) => {
  if (e.button === 0 && !e.shiftKey) {
    // selection happens on mouseup if the pointer did not move
  }
  S.dragging = true;
  S.dragMoved = false;
  S.dragStart = { x: e.clientX, y: e.clientY, vx: S.view.x, vy: S.view.y };
  if (e.button === 1 || e.button === 2 || e.shiftKey) {
    canvas.style.cursor = "grabbing";
    e.preventDefault();
  }
});

canvas.addEventListener("mousemove", (e) => {
  const rect = canvas.getBoundingClientRect();
  const mx = e.clientX - rect.left;
  const my = e.clientY - rect.top;
  if (S.dragging) {
    const dx = e.clientX - S.dragStart.x;
    const dy = e.clientY - S.dragStart.y;
    if (Math.abs(dx) + Math.abs(dy) > 3) S.dragMoved = true;
    if (S.dragMoved) {
      S.view.x = S.dragStart.vx + dx;
      S.view.y = S.dragStart.vy + dy;
      draw();
    }
    return;
  }
  const p = screenToWorld(mx, my);
  const hit = hitTest(p.x, p.y);
  if (hit !== S.hover) {
    S.hover = hit;
    draw();
  }
  const tip = $("tooltip");
  if (hit) {
    tip.classList.remove("hidden");
    tip.style.left = Math.min(rect.width - 320, mx + 14) + "px";
    tip.style.top = Math.min(rect.height - 90, my + 14) + "px";
    tip.innerHTML =
      '<div class="t-id">#' + hit.id + " · " + esc(hit.type) + "</div>" +
      '<div class="t-sub">' +
      esc(controlName(hit) || "(no header name)") +
      " · " + hit.rect.join(", ") + " · " +
      esc(hit.status) + "</div>";
  } else {
    tip.classList.add("hidden");
  }
});

window.addEventListener("mouseup", (e) => {
  if (!S.dragging) return;
  S.dragging = false;
  canvas.style.cursor = "crosshair";
  if (S.dragMoved) return;
  if (e.target !== canvas) return;
  const rect = canvas.getBoundingClientRect();
  const p = screenToWorld(e.clientX - rect.left, e.clientY - rect.top);
  const hit = hitTest(p.x, p.y);
  if (hit) {
    selectControl(hit);
  }
});

canvas.addEventListener("mouseleave", () => {
  S.hover = null;
  $("tooltip").classList.add("hidden");
  draw();
});

// ---------------------------------------------------------------------------
// selection / inspector
// ---------------------------------------------------------------------------
function applyHash() {
  const deadMatch = location.hash.match(/dead=(-?\d+)/);
  if (deadMatch && S.data) {
    switchTab("dead");
    const value = Number(deadMatch[1]);
    const groups = $("deadList").querySelectorAll(".dead");
    for (const group of groups) {
      const head = group.querySelector(".d-id");
      if (head && Number(head.textContent) === value) {
        group.classList.add("open");
        group.scrollIntoView({ block: "center" });
        break;
      }
    }
    return;
  }
  const m = location.hash.match(/id=(\d+)/);
  if (!m) return;
  const c = S.byId.get(Number(m[1]));
  if (c) selectControl(c, { center: true, noHash: true });
}

window.addEventListener("hashchange", applyHash);

function selectControl(c, options) {
  if (!c) return;
  if (!(options && options.noHash)) {
    history.replaceState(null, "", "#id=" + c.id);
  }
  S.selected = c;
  S.focus.clear();
  if (options && options.focus) {
    for (const value of options.focus) S.focus.add(value);
  }
  if (options && options.center) centerOn(c);
  renderInspector(c);
  updateListSelection();
  draw();
}

function updateListSelection() {
  let hit = null;
  $("list").querySelectorAll(".item").forEach((el) => {
    const on = !!S.selected && Number(el.dataset.id) === S.selected.id;
    el.classList.toggle("selected", on);
    if (on) hit = el;
  });
  if (hit) hit.scrollIntoView({ block: "nearest" });
}

function centerOn(c) {
  const rect = canvas.parentElement.getBoundingClientRect();
  S.view.scale = Math.max(0.5, S.view.scale, 1.0);
  const [x, y, w, h] = c.rect;
  S.view.x = rect.width / 2 - (x + w / 2) * S.view.scale;
  S.view.y = rect.height / 2 - (y + h / 2) * S.view.scale;
  updateViewInfo();
}

function renderInspector(c) {
  const refs = S.refsByValue.get(c.id) || [];
  const fieldRows = Object.entries(c.fields)
    .map(([k, v]) => '<div class="k">' + esc(k) + '</div><div class="v">' + esc(v) + "</div>")
    .join("");
  const refHtml = refs.length
    ? '<div class="reflist">' + refs.map(refCard).join("") + "</div>"
    : '<div class="warn">No source reference points at this id.</div>';
  const kids = (c.children || []).map((id) => {
    const child = S.byId.get(id);
    const name = child ? (controlName(child) || "control") : "?";
    return '<button class="kid" data-child="' + id + '">#' + id + " " + esc(name) + "</button>";
  }).join(" ");
  const parent = c.parent ? S.byId.get(c.parent) : null;
  const sprite = c.sprite;
  $("inspector").innerHTML =
    '<div class="sec"><h3>Control</h3>' +
    '<div class="kv">' +
    '<div class="k">id</div><div class="v">' + c.id + "</div>" +
    '<div class="k">type</div><div class="v">' + esc(c.type) + "</div>" +
    '<div class="k">status</div><div class="v">' + badge(c.status) + "</div>" +
    '<div class="k">rect</div><div class="v">' + c.rect.join(", ") + "</div>" +
    '<div class="k">parent</div><div class="v">' +
      (parent ? '<button class="kid" data-child="' + parent.id + '">#' + parent.id + " " + esc(controlName(parent) || parent.type) + "</button>" : "—") +
    "</div>" +
    '<div class="k">texture set</div><div class="v">' + (c.type === "3DOBJ" ? "—" : c.tex) + (c.sprite && c.sprite.name ? " · " + esc(c.sprite.name) : "") + "</div>" +
    '<div class="k">sprite</div><div class="v">' + (sprite ? esc(sprite.file) + " · frame " + sprite.frame + " · crop " + sprite.rect.join(",") : "—") + "</div>" +
    (c.string_index != null ? '<div class="k">string</div><div class="v">' + c.string_index + " · " + esc(c.text) + "</div>" : "") +
    "</div></div>" +
    '<div class="sec"><h3>Header names</h3><div class="kv"><div class="k">names</div><div class="v">' +
      (c.names && c.names.length ? c.names.join("<br>") : "—") +
    "</div>" +
    (c.proposed_name ? '<div class="k">proposed</div><div class="v">' + esc(c.proposed_name) + "</div>" : "") +
    "</div></div>" +
    (kids ? '<div class="sec"><h3>Children (' + c.children.length + ')</h3><div class="kids">' + kids + "</div></div>" : "") +
    '<div class="sec"><h3>Source references (' + refs.length + ')</h3>' + refHtml + "</div>" +
    '<div class="sec"><h3>Fields</h3><div class="kv">' + fieldRows + "</div></div>";

  $("inspector").querySelectorAll("[data-child]").forEach((el) => {
    el.addEventListener("click", () => {
      const child = S.byId.get(Number(el.dataset.child));
      if (child) selectControl(child, { center: true });
    });
  });
}

function refCard(ref) {
  return (
    '<div class="ref">' +
    '<div class="r-head">' + badge(ref.status || "computed") +
    '<span class="r-line">L' + ref.line + "</span>" +
    '<span class="r-fn">' + esc(ref.function || "") + "</span></div>" +
    "<code>" + esc(ref.snippet || ref.expr || "") + "</code></div>"
  );
}

// ---------------------------------------------------------------------------
// filtering / list
// ---------------------------------------------------------------------------
function matchesQuery(c) {
  const q = S.query.trim().toLowerCase();
  if (!q) return true;
  if (String(c.id).includes(q)) return true;
  if ((c.names || []).some((n) => n.toLowerCase().includes(q))) return true;
  if ((c.proposed_name || "").toLowerCase().includes(q)) return true;
  if ((c.text || "").toLowerCase().includes(q)) return true;
  if ((c.type || "").toLowerCase().includes(q)) return true;
  return false;
}

function matchesFilter(c) {
  const f = S.filter;
  if (f === "all") return true;
  if (f === "problems") return c.status !== "resolved";
  if (f === "orphans") return c.status === "orphan";
  if (f === "unnamed") return c.status === "unnamed";
  if (f === "referenced") return c.status === "resolved";
  return c.type === f;
}

function filteredControls() {
  return S.controls.filter((c) => matchesFilter(c) && matchesQuery(c));
}

function filteredSet() {
  return new Set(filteredControls().map((c) => c.id));
}

function renderList() {
  const list = filteredControls();
  $("listCount").textContent = list.length + " / " + S.controls.length + " controls";
  const html = list.map((c) => {
    const name = controlName(c);
    return (
      '<div class="item' + (S.selected && S.selected.id === c.id ? " selected" : "") + '" data-id="' + c.id + '">' +
      '<span class="dot" style="background:' + (STATUS_COLORS[c.status] || "#78808f") + '"></span>' +
      '<span class="id">#' + c.id + "</span>" +
      '<span class="type">' + esc(c.type) + "</span>" +
      '<span class="name">' + esc(name || (c.text ? "“" + c.text + "”" : "")) + "</span>" +
      "</div>"
    );
  }).join("");
  $("list").innerHTML = html;
  $("list").querySelectorAll(".item").forEach((el) => {
    el.addEventListener("click", () => {
      const c = S.byId.get(Number(el.dataset.id));
      if (c) selectControl(c, { center: true });
    });
  });
}

$("search").addEventListener("input", (e) => {
  S.query = e.target.value;
  renderList();
  draw();
});
$("filter").addEventListener("change", (e) => {
  S.filter = e.target.value;
  renderList();
  draw();
});
$("showLabels").addEventListener("change", (e) => {
  S.labels = e.target.checked;
  draw();
});
$("showOutlines").addEventListener("change", (e) => {
  S.outlines = e.target.checked;
  draw();
});
$("fitBtn").addEventListener("click", fit);

// ---------------------------------------------------------------------------
// tabs
// ---------------------------------------------------------------------------
function switchTab(name) {
  S.tab = name;
  document.querySelectorAll(".tab").forEach((b) => b.classList.toggle("active", b.dataset.tab === name));
  document.querySelectorAll(".tab-page").forEach((p) => p.classList.remove("active"));
  $("tab-" + name).classList.add("active");
}

document.querySelectorAll(".tab").forEach((btn) => {
  btn.addEventListener("click", () => switchTab(btn.dataset.tab));
});

// ---------------------------------------------------------------------------
// dead references
// ---------------------------------------------------------------------------
function renderDead() {
  const dead = S.data.dead_references || [];
  if (!dead.length) {
    $("deadList").innerHTML = '<div class="list-count">No dead references.</div>';
    return;
  }
  $("deadList").innerHTML = dead.map((group, idx) => {
    const ident = group.value != null ? group.value : group.token;
    const cands = (group.candidates || []).map((cand) => {
      const cls = cand.score >= 45 ? "strong" : cand.score >= 30 ? "medium" : "weak";
      return '<span class="cand ' + cls + '" data-cand="' + cand.id + '" data-group="' + idx + '">' +
        "#" + cand.id + " " + esc(cand.name || cand.type) + (cand.used ? " · used" : "") +
        '<span class="s">' + cand.score + "</span></span>";
    }).join("");
    const refs = group.references.map((ref) =>
      '<div class="prob-row" data-line="' + ref.line + '">L' + ref.line + " · " + esc(ref.function) +
      ' <span class="muted">' + esc(ref.snippet) + "</span></div>"
    ).join("");
    return (
      '<div class="dead" data-group="' + idx + '">' +
      '<div class="d-head">' +
      '<span class="d-id">' + esc(ident) + "</span>" +
      badge(group.category) +
      '<span class="d-count">' + group.references.length + " refs</span>" +
      "</div>" +
      '<div class="d-body">' +
      (group.note ? '<div class="d-note">' + esc(group.note) + "</div>" : "") +
      (cands ? '<div class="d-note">candidates (click to inspect):</div><div class="cands">' + cands + "</div>" : "") +
      refs +
      "</div></div>"
    );
  }).join("");

  $("deadList").querySelectorAll(".d-head").forEach((el) => {
    el.addEventListener("click", () => el.parentElement.classList.toggle("open"));
  });
  $("deadList").querySelectorAll(".cand").forEach((el) => {
    el.addEventListener("click", (e) => {
      e.stopPropagation();
      const c = S.byId.get(Number(el.dataset.cand));
      const group = S.data.dead_references[Number(el.dataset.group)];
      const focus = (group.candidates || []).map((x) => x.id);
      if (c) selectControl(c, { center: true, focus });
    });
  });
}

// ---------------------------------------------------------------------------
// problems
// ---------------------------------------------------------------------------
function renderProblems() {
  const m = S.data.meta.summary;
  const parts = [];
  parts.push('<div class="prob-block"><h3>Summary</h3><div class="kv">' +
    '<div class="k">references</div><div class="v">' + m.references + "</div>" +
    '<div class="k">resolved</div><div class="v">' + m.resolved_references + "</div>" +
    '<div class="k">dead values</div><div class="v">' + m.dead_values + "</div>" +
    '<div class="k">dead refs</div><div class="v">' + m.dead_references + "</div>" +
    '<div class="k">candidates</div><div class="v">' + m.dead_values_with_candidates + "</div>" +
    '<div class="k">orphans</div><div class="v">' + m.orphan_controls + "</div>" +
    '<div class="k">unnamed</div><div class="v">' + m.unnamed_controls + "</div>" +
    '<div class="k">type mismatch</div><div class="v">' + m.type_mismatches + "</div>" +
    "</div></div>");

  const unknown = (S.data.references || []).filter((r) => r.status === "unknown-constant");
  if (unknown.length) {
    parts.push('<div class="prob-block"><h3>Unknown constants / variables (' + unknown.length + ")</h3>" +
      unknown.map((r) => '<div class="prob-row">L' + r.line + " · " + esc(r.token) + " · " + esc(r.function) + "</div>").join("") +
      "</div>");
  }

  if ((S.data.type_mismatches || []).length) {
    parts.push('<div class="prob-block"><h3>Type mismatches</h3>' +
      S.data.type_mismatches.map((t) =>
        '<div class="prob-row" data-ctrl="' + t.value + '">#' + t.value + " · " + esc(t.token) +
        " · expected " + esc(t.expected) + " got " + esc(t.actual) + "</div>").join("") +
      "</div>");
  }

  if ((S.data.computed_references || []).length) {
    parts.push('<div class="prob-block"><h3>Computed references</h3>' +
      S.data.computed_references.map((r) =>
        '<div class="prob-row">L' + r.line + " · " + esc(r.expr) + " · base " + r.base +
        " · " + r.count + " ids · " + r.missing + " missing</div>").join("") +
      "</div>");
  }

  const unnamed = S.data.unnamed_controls || [];
  if (unnamed.length) {
    parts.push('<div class="prob-block"><h3>Unnamed scene ids (' + unnamed.length + ")</h3>" +
      unnamed.map((u) =>
        '<div class="prob-row" data-ctrl="' + u.id + '">#' + u.id + " · " + esc(u.type) +
        " · " + esc(u.proposed_name || "(no proposal)") + "</div>").join("") +
      "</div>");
  }

  const dups = S.data.meta.duplicate_constants || [];
  if (dups.length) {
    parts.push('<div class="prob-block"><h3>Constants sharing a value (' + dups.length + ")</h3>" +
      dups.map((d) => '<div class="prob-row">' + d.value + " · " + esc(d.names.join(", ")) + "</div>").join("") +
      "</div>");
  }

  const tex = S.data.meta.texture_problems || [];
  if (tex.length) {
    parts.push('<div class="prob-block"><h3>Texture problems (' + tex.length + ")</h3>" +
      tex.map((t) => '<div class="warn">#' + t.id + " · " + esc(t.message) + "</div>").join("") +
      "</div>");
  }

  const warnings = [].concat(
    (S.data.meta.scene_warnings || []).map((w) => "scene: " + w),
    (S.data.meta.asset_warnings || []).map((w) => "asset: " + w),
    (S.data.meta.source_warnings || []).map((w) => "source: " + w)
  );
  if (warnings.length) {
    parts.push('<div class="prob-block"><h3>Warnings</h3>' +
      warnings.map((w) => '<div class="warn">' + esc(w) + "</div>").join("") +
      "</div>");
  }

  const orphans = S.data.orphan_controls || [];
  if (orphans.length) {
    parts.push('<div class="prob-block"><h3>Source-unreferenced controls (' + orphans.length + ")</h3>" +
      orphans.slice(0, 120).map((o) =>
        '<div class="prob-row" data-ctrl="' + o.id + '">#' + o.id + " · " + esc(o.type) +
        " · " + esc(o.rect.join(", ")) + "</div>").join("") +
      (orphans.length > 120 ? '<div class="warn">… ' + (orphans.length - 120) + " more (use the Controls filter)</div>" : "") +
      "</div>");
  }
  $("problems").innerHTML = parts.join("");
  $("problems").querySelectorAll("[data-ctrl]").forEach((el) => {
    el.addEventListener("click", () => {
      const c = S.byId.get(Number(el.dataset.ctrl));
      if (c) selectControl(c, { center: true });
    });
  });
}

// ---------------------------------------------------------------------------
// keyboard / lifecycle
// ---------------------------------------------------------------------------
window.addEventListener("keydown", (e) => {
  if (e.target && /INPUT|SELECT|TEXTAREA/.test(e.target.tagName)) return;
  if (e.key === "f" || e.key === "F") fit();
  if (e.key === "l" || e.key === "L") {
    S.labels = !S.labels;
    $("showLabels").checked = S.labels;
    draw();
  }
  if (e.key === "o" || e.key === "O") {
    S.outlines = !S.outlines;
    $("showOutlines").checked = S.outlines;
    draw();
  }
  if (e.key === "/") {
    e.preventDefault();
    $("search").focus();
  }
  if (e.key === "Escape") {
    S.selected = null;
    S.focus.clear();
    $("inspector").innerHTML = '<div class="empty">Select a control or a dead reference.</div>';
    renderList();
    draw();
  }
});

window.addEventListener("resize", resize);
