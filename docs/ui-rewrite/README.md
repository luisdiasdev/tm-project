# UI Rewrite — Evaluation and Plan

This folder answers one question: **should the UI system be changed, and what
would it cost?**

It is based on a direct read of the current implementation (not the docs), a
parse of the three shipped 748 scene binaries, and the existing audit output in
`tools/fieldscene/out/`. All numbers below are reproducible; see
[`06-measurements.md`](06-measurements.md).

## TL;DR

The current UI is a hand-rolled retained-mode widget tree. It is not "archaic"
because it is retained-mode — that part is normal. It is painful because four
things are fused together in C++ source:

1. **layout** (`.bin` records plus hard-coded position fixes),
2. **style** (texture sets, colors, fonts),
3. **binding** (2,197 magic numeric IDs looked up by linear tree search),
4. **behavior** (a 3,521-line `OnControlEvent` if/else chain).

A new file format alone fixes only #1 and #2. Most of the day-to-day pain
(broken references, crashes on 755-only controls, multi-resolution drift,
rebuild-and-relogin iteration) comes from #3 and #4.

**Verdict: yes, change it — but as a staged data-driven refactor, not a
big-bang rewrite and not a third-party UI framework for the game screens.**

| Proposal | Impact | Cost | Risk | Score |
|---|---:|---:|---:|---:|
| Use a modern framework (RmlUi/Qt) for the **game UI** | 7 | 10 | 9 | **3/10** |
| Use Dear ImGui as a **developer overlay/inspector** | 8 | 2 | 2 | **9/10** |
| Make the UI more dynamic / declarative | 8 | 6 | 4 | **7/10** |
| **JSON** scene format | 8 | 4 | 3 | **8/10** |
| YAML scene format | 7 | 4 | 5 | **6/10** |
| Full big-bang rewrite | 7 | 10 | 9 | **2/10** |
| **Staged data-driven refactor (recommended)** | 9 | 6 | 4 | **9/10** |

## Complaint → cause → fix

| Your complaint | Actual cause | What fixes it | When |
|---|---|---|---|
| Not friendly to changes | `.bin` is write-only; layout lives in code | JSON scenes + editor | Phase 1, 8 |
| Error-prone | 2,197 magic IDs, no validation, silent failures | names + typed binds + validator | Q1, Q3, Phase 2–3 |
| Hard to troubleshoot | no inspector, failures are silent | ImGui hover inspector + fail-loud loader | Q2, Phase 2 |
| Very time-consuming | rebuild → relaunch → relogin for every pixel | hot reload + codegen | Q5, Phase 3–4 |

## Effort at a glance

Estimates are **developer-weeks** for one engineer who already knows this
codebase (1 week = 5 focused days). "AI-assisted" wall-clock is typically
0.6–0.8× the upper bound, but the low bound does not shrink much.

| Scope | Effort | What you get |
|---|---:|---|
| **Quick wins** (validator in CI, names, typed binds, ImGui inspector) | **3.5–6 wk** | Dead references and silent misbinds become build/runtime errors; you can see what you are clicking. No format change. |
| **MVP** (JSON scenes + typed bindings + hot reload) | **13–20 wk** | Change layout/art/text/visibility and see it live without a rebuild. Most "interface change" work stops touching C++. |
| **Core** (MVP + layout engine + new text pipeline) | **21.5–34 wk** | Resolution-independent layout, batched text, no GDI-per-label. |
| **Full** (Core + editor + declarative actions + parity) | **34.5–54 wk** | A real UI pipeline with a designer workflow. |
| Framework swap instead (RmlUi) | **16–28 wk** | Similar calendar cost, much higher risk of losing the 748 look and grid drag/drop. |

The full range is large because the honest answer depends on how much
pixel-parity you demand and whether an editor is in scope. The MVP is the
sweet spot: it targets exactly the complaints in the brief.

## The one thing to understand first

"Don't change code for interface changes" is achievable for **presentation**,
not for **gameplay**. A declarative format can move this:

- position/size/anchor, texture set and frame, color, font, text, visibility,
  draw order, grid shape, tab groups, tooltips, show/hide/toggle/set-text/
  play-sound/send-fixed-packet on click.

It cannot move this:

- inventory page math, item drag/drop validation, chat command parsing, combat
  and skill flows, server-driven state machines.

In the field scene, 91 of 139 top-level event branches are "simple-ish" (no
packet/game logic), so roughly **65% of click handlers can become data**. The
remaining ~35% stay code, but they should become small named handlers in a
registry instead of living in one 3,500-line function.

## Reading order

1. [`01-current-architecture.md`](01-current-architecture.md) — how it actually
   works today, end to end.
2. [`02-pain-points.md`](02-pain-points.md) — the failure modes, with evidence
   and a cost for each.
3. [`03-proposals.md`](03-proposals.md) — evaluation of your three proposals
   and the framework options.
4. [`04-refactor-plan.md`](04-refactor-plan.md) — staged plan, effort
   breakdown, risks, go/no-go gates.
5. [`05-target-format.md`](05-target-format.md) — concrete JSON schema,
   binding registry, actions, codegen, tooling.
6. [`06-measurements.md`](06-measurements.md) — raw numbers and how to
   reproduce them.

## What this is not

- Not a proposal to port the renderer away from D3D9. The renderer stays.
- Not a proposal to change the 748 look. Parity is a hard requirement.
- Not a proposal to drop the `.bin` files. They stay as the oracle and as the
  conversion source; JSON becomes the authoring format.
