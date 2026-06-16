# patchManager — Patch library (SQLite-backed)

**Filled in at:** M2.1 (PatchDB wired) — copy gearmulator's `jucePluginLib/patchdb/*` (per D-01 = copy + shim) with the dsp56kBase logging/threading utilities replaced by std/JUCE equivalents.

**Contents (planned):**
- PatchDB schema + migrations (sqlite_orm)
- Datasource management (auto-scan folders)
- Patch import/export (.syx / .mid)
- Search / tag / rating queries
- Browser UI (M2.2) — reimplemented JUCE-native rather than borrowed from gearmulator's RmlUi patch-manager (D-01)
