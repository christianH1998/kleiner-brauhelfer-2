# GUI-Schicht (`kleiner-brauhelfer/`)

## Start & Hauptfenster
- `main.cpp`: Settings (INI `kleiner-brauhelfer-2.ini`), DB-Wahl/-Kopie aus Ressource, Versionsprüfung + Update, `bh = new Brauhelfer(path)`, Ressourcen kopieren, `MainWindow`.
- `mainwindow.cpp/.ui`: `QTabWidget tabMain` mit `TabSudAuswahl`, `TabRezept`, `TabBraudaten`, `TabGaerverlauf1` (Hauptgärung), `TabAbfuellen`, `TabGaerverlauf2` (Nachgärung). Tabs erhalten `setup(mSud)`.
- Dialoge über Toolbar-Actions: `DlgAbstract::showDialog<DlgX>(this, action)` (nicht-modal, max. eine Instanz je Typ über statisches `DLG::Dialog`, `dialogs/dlgabstract-inl.h`). Beispiele: `DlgRohstoffe`, `DlgAusruestung`, `DlgBrauUebersicht`, `DlgPrintout`, `DlgEtikett`, `DlgBewertungen`, `DlgEinstellungen`, `DlgDatenbank`.
- Speichern/Verwerfen: `MainWindow::saveDatabase/discardDatabase` → `bh->save()/discard()`.
- Module ein-/ausblendbar: `Settings::Module` Flags (`settings.h`), `modulesChanged()` in Tabs. `gSettings->ForceEnabled` hebelt Status-Sperre aus („Eingabefelder entsperren“).

## Bindung UI ↔ Sud
- `SudObject mSud` (core) hält aktuelle SudID/Zeile, Getter/Setter je Sud-Spalte (`getSW()`, `setSudhausausbeute()` …, Makro `Q_PROPERTY_SUD`) und `ProxyModel`s der Kindtabellen (`mSud->modelMalzschuettung()` …).
- Sud laden: `TabSudAuswahl::doubleClicked` → `MainWindow::loadSud` → `mSud->load(id)` → `loadedChanged`.
- Einfache Felder: `DoubleSpinBoxSud`/`SpinBoxSud` (`widgets/`) mit `setColumn(mSud, ModelSud::ColX)`; schreiben nur bei Fokus via Undo-Command; `updateValue()` in `updateValues()` der Tabs.
- Listen von Gaben: dynamische Widgets `WdgMalzGabe`, `WdgHopfenGabe`, `WdgHefeGabe`, `WdgWeitereZutatGabe`, `WdgRast`, `WdgWasseraufbereitung` (Basis `WdgAbstractProxy`: `data(col)`, `setData(col,v)` → Undo, `remove/moveUp/moveDown`). Aufbau in `TabRezept::*_modified()` bei `rowsInserted/Removed/layoutChanged`.
- Neue Gabe: `DlgRohstoffAuswahl` (Rohstoff aus Lager wählen) → `mSud->modelX()->append({SudID, Name, …})`. Übernahme aus anderem Sud: `DlgUebernahmeRezept` + `bh->sudKopierenModel`.
- Fehlende Rohstoffe: `TabRezept::checkRohstoffe()` bietet `Model*Gaben::import()` an.
- Status-Sperren: `TabRezept::checkEnabled()` (Felder bei Status ≠ Rezept deaktiviert).

## Undo
- `commands/undostack.h` (`gUndoStack`), `SetModelDataCommand(model,row,col,value)` mit Merge. Abschaltbar per INI `[General] UndoEnabled`. Programmatische Massenänderungen (Import, Kopieren) laufen **ohne** Undo direkt über die Modelle.

## Darstellung/Ausgabe
- Charts: `charts/chart*.cpp` (QCustomPlot), z. B. `ChartMalz`, `ChartHopfen`, `ChartMaischplan` im Rezept-Tab.
- HTML-Ansichten (Spickzettel, Zusammenfassung, Sudinfo, Ausdruck): `templatetags.cpp` baut Mustache-Kontext (`Rezept`, `Sud`, `Zutaten`, `Maischplan`, `Wasser`, `Anlage`, `Tags`), Templates `data/Webview/*.html` (+ `_en/_nl/_sv`), Rendering `helper/mustache.cpp`, `widgets/wdgwebvieweditable`.
- Hilfe-Dock: Tooltip des fokussierten Widgets wird angezeigt (`MainWindow::focusChanged`).

## Konventionen
- UI in Qt Designer `.ui`; eigene Widgets als promoted custom widgets (`widgets/*.h`, `charts/*.h`).
- `WidgetDecorator::addDecorator(...)` am Ende von `setup()` (Hervorhebung geänderter Werte; bei programmatischen Massenänderungen `WidgetDecorator::suspendValueChangedClear = true/false` setzen, siehe `TabRezept::on_btnMalzAusgleichen_clicked`).
- UI-Texte deutsch mit `tr()`; Übersetzungen `translations/kbh_{en,nl,sv}.ts/.qm` (+ `.qph`), Templates je Sprache separat.
- Tabellen: `widgets/tableview.h` mit `appendCol({col, visible, canHide, width, delegate})` (`TableView::ColumnDefinition`) + Delegates aus `model/`.
- Neue Dateien in `kleiner-brauhelfer.pro` (`SOURCES`, `HEADERS`, `FORMS`) eintragen.
- Kennzeichnung (ab ext-s04): schreibgeschützte `DoubleSpinBox`/`SpinBox` = berechneter Wert ⇒ automatisch `gSettings->paletteBerechnet` (Priorität: geändert > Fehler > berechnet > normal). Berechnete Felder daher per `setReadOnly(true)` (nicht `setDisabled`) darstellen; `setDisabled` bleibt für Status-Sperren.
- Exakte Werte (ab ext-s01b): `DoubleSpinBoxSud`/`SpinBoxSud` schreiben beim Zurückstellen auf den Anzeigewert vom Fokusbeginn den exakten Modellwert zurück.
- Modus-Auswahlen (ab ext-s01): `ComboBox` mit Gesamt-Tooltip (alle Modi erklärt) + Tooltip je Eintrag via `setItemData(i, text, Qt::ToolTipRole)`; Index = Enum + Offset; Moduswechsel darf keine Werte ändern.
