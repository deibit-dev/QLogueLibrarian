# QLogueLibrarian — Diseño a nivel de clases

## Vista general

Arquitectura MVC adaptada a QML, con tres capas en `src/`:

- **`src/logic/` — Model + orquestador**: `Logic` es a la vez el **estado
  observable** (expuesto a QML, read-only) y el **único orquestador** de la
  lógica de negocio (probe / scan / load / settings). `model/` contiene los
  tipos puros (value types) que no orquestan nada.
- **`src/controller/` — gateway QML**: `AppController` es el **único camino**
  por el que las vistas modifican el modelo; valida, delega en `Logic` y
  traduce sus señales de operación a texto de UI (`statusText`/`logText`).
- **`src/view/` + `qml/` — Vista**: QML declarativo + `ViewState` (dueño de
  la selección / estado de presentación) + `Dialogs`.

Todo el código vive en el namespace `qlogue`. Reglas de oro:

1. **`Logic` es read-only para QML** (7 `Q_PROPERTY` sin `WRITE`): la vista
   bindea para mostrar; los comandos pasan solo por `AppController`.
2. **Un solo escritor lógico**: `Logic` se muta a sí mismo (negocio) y recibe
   el feedback de presentación vía `appendLog`/`setStatus` invocados por
   `AppController`. No hay dos caminos de escritura desde la vista.
3. **La selección es de la vista** (`ViewState`): `Logic` no guarda qué
   unit/puerto está elegido, ni el target de carga, ni el slot.
4. **`Logic` no compone cadenas de usuario**: emite señales de operación
   tipadas; `AppController` las traduce a mensajes.
5. **La orquestación de `Logic` no es `Q_INVOKABLE`**: QML nunca la ve.

## Capas

### `src/model/` (tipos puros, sin QObject de orquestación)

- **`MidiPort`** (struct, `MidiPort.h`): puerto MIDI reportado por
  `logue-cli probe -l`. `direction` (In/Out), `index`, `name`.
- **`UnitParam`** (value type, `UnitInfo.h`): un parámetro del manifest
  (`name`, `min`, `max`, `type`). `Q_GADGET`, campos privados + getters.
- **`UnitInfo`** (value type, `UnitInfo.h`): metadatos de un unit
  (`filePath`, `fileName`, `platform`, `module`, `api`, `devId`, `prgId`,
  `version`, `name`, `numParams`, `params`, `isValid`).
- **`KorgEnums.h`**: `unitExtensions()` (globs) y `unitFileFilter()`.

### `src/logic/` (negocio)

- **`LogueCLIWrapper`** (QObject, `LogueCLIWrapper.h/.cpp`): servicio
  **stateless** que envuelve `logue-cli` vía `QProcess`. Asíncrono: emite
  `probeFinished(QVector<MidiPort>)`, `loadFinished(LoadResult)`,
  `errorOccurred(QString)`. También define `LoadResult`.
- **`UnitHeaderParser`** (estática, `UnitHeaderParser.h/.cpp`):
  `parse(QString) → UnitInfo` (extrae `manifest.json` del ZIP con **miniz**).
  Único escritor de `UnitInfo` (usa sus setters).
- **`Logic`** (QObject, `Logic.h/.cpp`): **ex `AppState` + orquestador
  fusionados**. Es el agregado: posee su estado y las operaciones que lo
  mutan, de modo que no hay ambigüedad sobre quién escribe el modelo.
  - **7 `Q_PROPERTY`**, todas read-only para QML: `cliPath`, `unitDir`,
    `inPorts`, `outPorts`, `library`, `statusText`, `logText`.
  - **7 señales NOTIFY** (una por propiedad).
  - **Señales de operación** (para que `AppController` traduzca):
    `probeFinished(int)`, `scanFinished(int)`, `loadFinished(LoadResult)`,
    `errorOccurred(QString)`.
  - **Operaciones de negocio** (llamadas por `AppController`, no `Q_INVOKABLE`):
    `probe()`, `loadUnit(unitPath, slot, inRow, outRow)` (resuelve fila →
    índice MIDI crudo), `scanUnits(dir)`, `setCliPath(v)`, `setUnitDir(v)`.
  - **Feedback** (invocado por `AppController`): `appendLog()`, `setStatus()`.
  - **Miembro** `LogueCLIWrapper m_cli` (poseído por valor). Los resultados
    del wrapper mutan el estado propio (`setPorts`/`setUnits`) y luego emiten
    la señal de operación correspondiente.
  - **QSettings** interno: lee defaults en el ctor y persiste en
    `setCliPath`/`setUnitDir` (clave `library/lastDir`, sin cambios).
  - `m_units` es la única fuente de la librería; `library()` la proyecta a
    `QVariantList` de value types bajo demanda.

### `src/controller/`

- **`AppController`** (`AppController.h/.cpp`): **gateway QML delgado, sin
  estado propio**. Guarda `Logic *m_logic` y, en su ctor, conecta las señales
  de operación de `Logic` para traducirlas a `statusText`/`logText` (via
  `Logic::appendLog`/`setStatus`). Expone 4 `Q_INVOKABLE`:
  `probe()`, `setCliPath(path)`, `setUnitDir(dir)`, `loadUnit(unitPath,
  slot, inRow, outRow)`. Cada una valida y delega. En el ctor conserva el
  arranque: `probe()` inicial + `scanUnits` del directorio persistido.

### `src/view/` + `qml/`

- **`ViewState`** (`ViewState.h/.cpp`): QObject de la capa de vista que
  centraliza la selección / valores de formulario: `libraryIndex`, `unitPath`,
  `inIndex`, `outIndex`, `slot` y el derivado `canLoad`.
- **`Dialogs`** (`Dialogs.h/.cpp`): helper sin estado (`pickExecutable`,
  `pickUnitFile`, `pickDirectory`).
- **QML** (`Main.qml`, `MidiSection.qml`, `LoadSection.qml`,
  `UnitLibrary.qml`, `MetaPanel.qml`, `LogPanel.qml`): enlaza a `App`
  (estado, solo lectura) y a `ViewState` (selección); llama a `Controller`
  (comandos) y a `Dialogs`. Los campos de texto de settings se confirman con
  `editingFinished` invocando `Controller.setCliPath/setUnitDir`. `MetaPanel`
  lee la unit elegida con `App.library[ViewState.libraryIndex]`.

### `src/main.cpp`

Registra `UnitInfo`/`UnitParam` como value types, crea `Logic`,
`AppController(&logic)`, `ViewState` y `Dialogs`, y los expone como `App`,
`Controller`, `ViewState` y `Dialogs`.

## Dirección de dependencias

```
View (qml/ + ViewState + Dialogs) ──lee──▶ Logic (App, read-only)
      │
      │ llama (comandos)                    │ orquesta / se muta
      ▼                                     ▼
AppController (gateway) ───────delega────▶ Logic ──posee──▶ LogueCLIWrapper
      ▲ traducción de señales                     │
      └── (statusText/logText vía setters)        └─usa─▶ UnitHeaderParser
```

- La **vista** conoce a `Logic` (lectura) y a `AppController` (comandos).
- **Escrituras**: `Logic` es read-only para QML; el único camino de comando
  desde la vista es `AppController`. La mutación la ejecuta `Logic` sobre sí
  mismo.
- **`AppController`** conoce a `Logic` (y al modelo solo para feedback), pero
  nadie lo conoce a él salvo QML.
- `model/` no depende de nada; `logic/` depende de `model/`; `controller/`
  depende de `logic/`; `view/` lee `model`-expuesto-y-`logic` y llama al
  `controller`.

## Diagrama de clases

```
              ┌────────────────────┐
              │      main.cpp      │  registra UnitInfo/UnitParam (value types)
              └─┬────────┬─────────┘
        crea/App │        │ crea/Controller, ViewState, Dialogs
                 ▼        ▼
   ┌─────────────────────────────┐  ┌───────────────────────────┐
   │         Logic (QObject)     │  │      AppController        │  (QObject)
   │  Model + orquestador        │◀─│  (gateway QML, delgado)   │
   │  ────────────────────────── │  │  ──────────────────────── │
   │  7 Q_PROPERTY (read-only)   │  │  + probe()                │
   │  7 NOTIFY signals           │  │  + setCliPath(path)       │
   │  op signals: probe/scan/    │  │  + setUnitDir(dir)        │
   │    load/error               │  │  + loadUnit(unitPath,     │
   │  probe/loadUnit/scanUnits/  │  │    slot, inRow, outRow)   │
   │    setCliPath/setUnitDir    │  │  ──────────────────────── │
   │  appendLog/setStatus        │  │  - m_logic : Logic*       │
   │  ────────────────────────── │  └───────────┬───────────────┘
   │  m_cli : LogueCLIWrapper    │              │ delega
   │  m_inPorts/m_outPorts       │              ▼
   │  m_units (única fuente)     │   (traduce señales → status/log)
   │  m_cliPath/m_unitDir ...    │
   └─────────────────────────────┘
              │ posee            │ usa (parse)
              ▼                  ▼
   ┌──────────────────────┐  ┌────────────────────┐
   │   LogueCLIWrapper    │  │  UnitHeaderParser  │  (static)
   │   (QProcess, async)  │  └─────────┬──────────┘
   └──────────┬───────────┘            │ usa miniz
              │ spawns                 ▼
              ▼                 UnitInfo (value type)
   ┌──────────────┐            UnitParam ──(params)──▶
   │  logue-cli   │  emits MidiPort / LoadResult
   └──────────────┘

   View (qml/) ──lee──▶ App (Logic read-only)      ──lee/escribe──▶ ViewState
      │ llama ──▶ Controller (AppController)         (selección)
      │ llama ──▶ Dialogs (src/view/, QFileDialog)
```

## Notas sobre el flujo

1. **Inicio** (`main.cpp`): se crea `Logic` (restaura `QSettings`),
   `AppController(&logic)`, `ViewState` y `Dialogs`. El ctor de
   `AppController` conecta las señales de operación de `Logic` a la
   traducción de UI y conserva el arranque: `probe()` y `scanUnits` del
   directorio persistido si existe.
2. **Probe**: QML llama `Controller.probe()` → `AppController` pone
   "Probing…" y llama `Logic::probe()` → `LogueCLIWrapper::probe(cliPath)` →
   `probeFinished` → `Logic` hace `setPorts(ports)` y emite
   `probeFinished(n)` → `AppController` escribe "Found N port(s)" en
   `statusText`/`logText`. La auto-selección de "SOUND" es de la vista
   (`MidiSection.qml` elige `ViewState.inIndex/outIndex`).
3. **Exploración de la librería**: QML llama `Controller.setUnitDir(dir)`
   (o confirma el campo con `editingFinished`) → `Logic::setUnitDir` persiste
   y hace `scanUnits(dir)` → `scanFinished(n)` → `AppController` traduce
   "Unit library: N unit(s) found". Al cambiar la librería, la vista resetea
   `ViewState.libraryIndex`.
4. **Selección de unit**: el `ListView` de `UnitLibrary.qml` actualiza
   `ViewState.libraryIndex` (highlight) y `ViewState.unitPath` (target).
   `MetaPanel` muestra la unit leyendo `App.library[ViewState.libraryIndex]` —
   el modelo sigue siendo la única fuente de verdad.
5. **Carga (upload)**: QML llama `Controller.loadUnit(ViewState.unitPath,
   ViewState.slot, ViewState.inIndex, ViewState.outIndex)`. `AppController`
   valida, llama `Logic::loadUnit` (resuelve fila → índice MIDI crudo) y pone
   "Uploading…". Al terminar, `loadFinished(LoadResult)` → `AppController`
   apenda `rawOutput` al log y actualiza el estado (✔/✘).
