# Patch-Info: wt-p19

- **Issue:** windirstat/windirstat#634 (Folge von #593, PR #604)
- **Branch:** feat/search-empty-folders-tree
- **Erstellt:** 2026-07-28
- **Status:** In Arbeit
- **Ziel:** Zweite Variante zu wt-p18 (Branch `feat/search-empty-folders`). Gleiches Grundprinzip (Leere-Ordner-Suche zeigt Ergebnisse in der Suchergebnis-Ansicht statt sofort zu loeschen, Wiederverwendung von `DeletePhysicalItems`), aber statt verschachtelte leere Unterordner wegzulassen (Topmost-Only, wt-p18), werden sie **echt hierarchisch verschachtelt** angezeigt (z.B. `empty_b_sub` sichtbar als Kind von `empty_b`, mit Aufklapp-Pfeil), damit nichts an Information verloren geht und die Zugehoerigkeit fuer den Nutzer sichtbar bleibt (DoMe: "die Baumstruktur ist ehrlicher").
- **Unterschied zu wt-p18:** Traversal bleibt vollstaendig (steigt weiter in bereits-leere Ordner ab, keine Verkuerzung), aber die gefundenen Elemente werden beim Anzeigen anhand ihrer echten Eltern-Kind-Beziehung verschachtelt statt alle flach unter die Suchergebnis-Wurzel zu haengen. Erfordert zusaetzlich eine Korrektur an `CFileSearchControl::RemoveItem`, die aktuell hart annimmt, dass jedes Suchergebnis ein direktes Kind der Wurzel ist.
- **Vorgehen:** Beide Varianten (wt-p18 flach/topmost-only, wt-p19 hierarchisch) parallel bauen und DoMe zum direkten Vergleich vorlegen, bevor eine Richtung fuer Issue #634 gewaehlt wird. Noch nichts posten.
