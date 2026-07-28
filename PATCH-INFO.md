# Patch-Info: wt-p18

- **Issue:** windirstat/windirstat#634 (Folge von #593, PR #604)
- **Branch:** feat/search-empty-folders
- **Erstellt:** 2026-07-28
- **Status:** In Arbeit
- **Ziel:** "Remove Empty Folders" zu "Search For Empty Folders" umbauen — Suche zeigt gefundene leere Ordner in der bestehenden Suchergebnis-Ansicht (`CFileSearchControl`), Nutzer waehlt einzelne Ordner aus (Mehrfachauswahl wie bei normaler Suche), Loeschen/Papierkorb danach ueber die bestehende `DeletePhysicalItems`-Funktion statt Neuimplementierung.
- **Bezug:** Nicht verwandt mit dem alten Branch `feat/safe-remove-empty-folders` (wt-p17, PR #604) — der ist ueberholt, da NoMoreFood den Bestaetigungsdialog-Teil am 2026-07-24 selbst per Commit `581c609d` ("Destructive Action Confirmation") geloest hat. wt-p17 bleibt unangetastet liegen.
- **Vorgehen:** Erst Prototyp im Fork bauen und lokal testen, bevor irgendwas im Issue gepostet wird (siehe Memory `how_windirstat_works_SoT.md` und `project_nomorefood_style.md`).
