# QLogueLibrarian

**Author:** David Emmanuel Lopez

A minimal cross-platform GUI wrapper for KORG logue-sdk/logue-cli tools, built with **Qt6**, **QtQuick/QML** and **CMake**. It wraps the official [`logue-cli`](https://github.com/korginc/logue-sdk/tree/main/tools/logue-cli) command-line tool to provide a graphical interface for probing MIDI ports and uploading custom oscillator/effect units to your device.

![example](/resources/capture.png)

---

## Motivation

- **No Linux GUI exists.** Korg Sound Librarian is a Windows/MAC only application. There is no official (or unofficial) equivalent for Linux users.
- **MIDI just works on Linux.** Korg's USB-MIDI drivers are notoriously problematic on Windows (driver conflicts, unsigned driver warnings, etc.). On Linux, logue-series devices are recognized out-of-the-box as standard ALSA MIDI devices — no driver installation required.

## Current Features

| Feature | Description |
|---|---|
| **Probe MIDI ports** | Runs `logue-cli probe -l`, parses the output and populates In/Out port selectors. Ports containing `SOUND` are auto-selected (these are the correct SysEx ports for prologue / minilogue xd). |
| **Upload unit files** | Lets you browse for a `.prlgunit`, `.mnlgxdunit`, `.ntkdigunit` (or other logue unit format), then runs `logue-cli load -u <file> -i <in> -o <out>`. Parses the output to determine success (platform, module, CRC32). |
| **Plugin Library browser** | Point the app at a directory containing `.xxxunit` files. All plugins are listed and selecting one displays its metadata and automatically fills the Load Unit path for quick upload. |
| **Manifest metadata parsing** | Unit files (`.prlgunit`, `.mnlgxdunit`, etc.) are ZIP archives containing a `manifest.json`. The app extracts it via the bundled **miniz** library (vendored as a git submodule) and displays: platform, module, API version, developer/program IDs, unit name, and a full parameter table (name, min, max, type). |
| **Persistent settings** | The path to the `logue-cli` executable and the plugin library directory are saved via `QSettings` and restored on next launch. The plugin directory is automatically scanned on startup. |
| **Live log** | All `logue-cli` stdout/stderr output is shown in a log panel inside the application. |

## Project Structure

```
src/
├── main.cpp                         # Application entry point (QQmlApplicationEngine)
├── model/
│   ├── KorgEnums.h                  # Unit file extension filters
│   ├── UnitInfo.h/.cpp              # Data model for unit metadata (from manifest.json)
│   └── PluginListModel.h/.cpp       # QAbstractListModel exposed to QML ListView
├── controller/
│   ├── LogueCLIWrapper.h/.cpp       # QProcess wrapper for logue-cli
│   ├── UnitHeaderParser.h/.cpp      # Extracts & parses manifest.json from .xxxunit ZIP files
│   └── AppController.h/.cpp         # C++ backend (Q_PROPERTY / Q_INVOKABLE) exposed to QML
└── qml/
    ├── Main.qml                     # Root ApplicationWindow
    ├── MidiSection.qml              # Probe + In/Out ComboBoxes
    ├── LoadSection.qml              # Unit file picker + slot + upload button
    ├── PluginLibrary.qml            # Directory picker + SplitView (list + metadata)
    ├── MetaPanel.qml                # Metadata grid + parameter table
    └── LogPanel.qml                 # Live log (read-only)
```

## Prerequisites

### Build Dependencies

| Package | Minimum Version |
|---|---|
| CMake | 3.20 |
| GCC (or Clang) | GCC 9+ / Clang 10+ (C++17) |
| Qt6 (Core, Quick, QuickControls2) | 6.2 |
| **miniz** | Vendored as git submodule (`external/miniz`, pinned to tag `3.0.2`) |

> **Note:** this project uses a git submodule. Clone it with:

> ```bash
> git clone --recurse-submodules https://github.com/<you>/QLogueLibrarian.git
> ```

> Or, if you already cloned it without submodules:

> ```bash
> git submodule update --init --recursive
> ```

### Runtime Dependencies

The only runtime dependency is `logue-cli` itself (see below). The **miniz** library used to read `.xxxunit` ZIP archives is vendored in this repository and compiled into the application, so no external ZIP/unzip tool is required.

**Debian / Ubuntu:**

```bash
sudo apt install build-essential cmake qt6-base-dev qt6-declarative-dev
```

**Fedora:**

```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtdeclarative-devel
```

**Arch Linux:**

```bash
sudo pacman -S base-devel cmake qt6-base qt6-declarative
```

### Installing logue-cli

`logue-cli` is the official Korg command-line utility for communicating with logue-series devices. It is **not** included in this repository — you need to download it separately.

**Linux (automatic):**

```bash
git clone https://github.com/korginc/logue-sdk.git
cd logue-sdk/tools/logue-cli
./get_logue_cli_linux.sh
```

This downloads and unpacks `logue-cli` into `logue-sdk/tools/logue-cli/logue-cli-linux64-0.07-2b/`.

**macOS:**

```bash
cd logue-sdk/tools/logue-cli
./get_logue_cli_osx.sh
```

**Windows (MSYS2):**

```bash
cd logue-sdk/tools/logue-cli
./get_logue_cli_msys.sh
```

For manual Windows downloads, see the [official logue-cli README](https://github.com/korginc/logue-sdk/blob/main/tools/logue-cli/README.md).

Once installed, point the application to the `logue-cli` executable using the path field at the top of the window. The path is remembered between sessions.

## Building & Running

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/QLogueLibrarian
```

## Disclaimer

**This project is not affiliated with, endorsed by, or related to KORG Inc. in any way.** It is an independent, open-source GUI that acts as a wrapper around the officially published `logue-cli` tool.

> ⚠️ **Important licensing note:** `logue-cli` is a **proprietary binary** distributed by KORG under its own End User License Agreement (EULA). It is **not** open-source and is **not** covered by the BSD 3-Clause license of the [logue-sdk](https://github.com/korginc/logue-sdk) source code. This project does not bundle, redistribute, or modify `logue-cli` in any way — it only invokes it as an external process. You must obtain `logue-cli` directly from KORG and comply with their EULA when using it.

All product names and trademarks mentioned are the property of their respective owners.

## License

This project is licensed under the **GNU General Public License v3.0**. See [LICENSE](LICENSE) for details.

---
---

# QLogueLibrarian (Español)

**Autor:** David Emmanuel Lopez

Interfaz gráfica minimalista y multiplataforma que actúa como wrapper de las herramientas KORG logue-sdk/logue-cli, construida con **Qt6**, **QtQuick/QML** y **CMake**. Funciona como wrapper de la herramienta oficial [`logue-cli`](https://github.com/korginc/logue-sdk/tree/main/tools/logue-cli).


## Motivación

- **No existe una GUI similar a Korg Sound Librarian para Linux.** La aplicación oficial de Korg es exclusiva de Windows/MAC. No hay alternativa gráfica oficial ni de terceros para usuarios de Linux.
- **Los drivers MIDI de Korg son conflictivos en Windows.** En Linux, los dispositivos de la serie logue son reconocidos automáticamente como dispositivos ALSA MIDI estándar, sin necesidad de instalar drivers.

## Funcionalidad Actual

| Función | Descripción |
|---|---|
| **Detección de puertos MIDI** | Ejecuta `logue-cli probe -l`, parsea la salida y llena los selectores de puertos In/Out. Los puertos que contienen `SOUND` se seleccionan automáticamente (son los puertos SysEx correctos para prologue / minilogue xd). |
| **Subida de archivos unit** | Permite seleccionar un archivo `.prlgunit`, `.mnlgxdunit`, `.ntkdigunit` (u otro formato logue), luego ejecuta `logue-cli load -u <archivo> -i <in> -o <out>`. Parsea la salida para determinar éxito (plataforma, módulo, CRC32). |
| **Explorador de librería de plugins** | Apuntá la app a un directorio que contenga archivos `.xxxunit`. Todos los plugins se listan y al seleccionar uno se muestran sus metadatos y se autocompleta la ruta en Load Unit para subirlo rápidamente. |
| **Parseo de metadatos (manifest.json)** | Los archivos unit (`.prlgunit`, `.mnlgxdunit`, etc.) son archivos ZIP que contienen un `manifest.json`. La app lo extrae con la librería **miniz** (incluida como git submodule) y muestra: plataforma, módulo, versión de API, IDs de desarrollador/programa, nombre de la unidad, y tabla completa de parámetros (nombre, mín, máx, tipo). |
| **Configuración persistente** | La ruta al ejecutable `logue-cli` y el directorio de la librería de plugins se guardan con `QSettings` y se restauran al reiniciar. El directorio de plugins se escanea automáticamente al iniciar. |
| **Log en vivo** | Toda la salida stdout/stderr de `logue-cli` se muestra en un panel de log dentro de la aplicación. |

## Estructura del Proyecto

```
src/
├── main.cpp                         # Punto de entrada (QQmlApplicationEngine)
├── model/
│   ├── KorgEnums.h                  # Filtros de extensiones de archivos unit
│   ├── UnitInfo.h/.cpp              # Modelo de datos para metadatos (desde manifest.json)
│   └── PluginListModel.h/.cpp       # QAbstractListModel expuesto al ListView de QML
├── controller/
│   ├── LogueCLIWrapper.h/.cpp       # Wrapper de QProcess para logue-cli
│   ├── UnitHeaderParser.h/.cpp      # Extrae y parsea manifest.json de archivos ZIP .xxxunit
│   └── AppController.h/.cpp         # Backend C++ (Q_PROPERTY / Q_INVOKABLE) expuesto a QML
└── qml/
    ├── Main.qml                     # ApplicationWindow raíz
    ├── MidiSection.qml              # Probe + ComboBoxes In/Out
    ├── LoadSection.qml              # Selector de archivo + slot + botón Upload
    ├── PluginLibrary.qml            # Selector de directorio + SplitView (lista + metadatos)
    ├── MetaPanel.qml                # Grilla de metadatos + tabla de parámetros
    └── LogPanel.qml                 # Log en vivo (solo lectura)
```

## Requisitos

### Dependencias de Compilación

| Paquete | Versión Mínima |
|---|---|
| CMake | 3.20 |
| GCC (o Clang) | GCC 9+ / Clang 10+ (C++17) |
| Qt6 (Core, Quick, QuickControls2) | 6.2 |
| **miniz** | Vendido como git submodule (`external/miniz`, fijado al tag `3.0.2`) |

> **Nota:** este proyecto usa un git submodule. Clonalo con:

> ```bash
> git clone --recurse-submodules https://github.com/<usuario>/QLogueLibrarian.git
> ```

> O, si ya lo clonaste sin submodules:

> ```bash
> git submodule update --init --recursive
> ```

### Dependencias de Ejecución

La única dependencia de ejecución es `logue-cli` (ver abajo). La librería **miniz**, usada para leer los ZIP `.xxxunit`, está **incluida** en este repositorio y se compila dentro de la aplicación — no se requiere ninguna herramienta externa de ZIP/unzip.

**Debian / Ubuntu:**

```bash
sudo apt install build-essential cmake qt6-base-dev qt6-declarative-dev
```

**Fedora:**

```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtdeclarative-devel
```

**Arch Linux:**

```bash
sudo pacman -S base-devel cmake qt6-base qt6-declarative
```

### Instalación de logue-cli

`logue-cli` es la utilidad oficial de línea de comandos de Korg para comunicarse con dispositivos de la serie logue. **No está incluida** en este repositorio — hay que descargarla por separado.

**Linux (automático):**

```bash
git clone https://github.com/korginc/logue-sdk.git
cd logue-sdk/tools/logue-cli
./get_logue_cli_linux.sh
```

Esto descarga y descomprime `logue-cli` en `logue-sdk/tools/logue-cli/logue-cli-linux64-0.07-2b/`.

**macOS:**

```bash
cd logue-sdk/tools/logue-cli
./get_logue_cli_osx.sh
```

**Windows (MSYS2):**

```bash
cd logue-sdk/tools/logue-cli
./get_logue_cli_msys.sh
```

Para descargas manuales en Windows, ver el [README oficial de logue-cli](https://github.com/korginc/logue-sdk/blob/main/tools/logue-cli/README.md).

Una vez instalado, configurá la ruta al ejecutable `logue-cli` en el campo de la parte superior de la ventana. La ruta se recuerda entre sesiones.

## Compilación y Ejecución

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/QLogueLibrarian
```

## Aviso Legal (Disclaimer)

**Este proyecto no está afiliado, respaldado ni relacionado con KORG Inc. de ninguna manera.** Es una GUI independiente y de código abierto que funciona como wrapper de la herramienta `logue-cli` publicada oficialmente por KORG.

> ⚠️ **Nota importante sobre licencias:** `logue-cli` es un **binario propietario** distribuido por KORG bajo su propio Acuerdo de Licencia de Usuario Final (EULA). **No es** software de código abierto y **no está** cubierto por la licencia BSD 3-Clause del código fuente del [logue-sdk](https://github.com/korginc/logue-sdk). Este proyecto no incluye, redistribuye ni modifica `logue-cli` de ninguna manera — únicamente lo invoca como proceso externo. Debés obtener `logue-cli` directamente de KORG y cumplir con su EULA al utilizarlo.

Todos los nombres de productos y marcas registradas mencionados son propiedad de sus respectivos dueños.

## Licencia

Este proyecto está licenciado bajo la **GNU General Public License v3.0**. Ver [LICENSE](LICENSE) para más detalles.
