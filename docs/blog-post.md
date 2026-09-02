# QLogueLibrarian: un wrapper GUI multiplataforma para KORG logue-cli

**Autor:** David Emmanuel Lopez  
**Stack:** C++17 / Qt6 / QtQuick QML / CMake  
**Licencia:** GPL v3

---

## Introduccion

`QLogueLibrarian` es una interfaz grafica minimalista que envuelve la herramienta oficial `logue-cli` de KORG para subir unidades (osciladores, efectos) a sintetizadores de la serie logue (minilogue xd, prologue, NTS-1, drumlogue, etc.). El problema original: no existe un equivalente de Korg Sound Librarian para Linux, y los drivers USB-MIDI de Korg son problematicos en Windows. En Linux los dispositivos logue son reconocidos out-of-the-box como dispositivos ALSA MIDI standard.

El codigo fuente esta disponible en [GitHub](https://github.com/david-lafontaine/QLogueLibrarian).

---

## Decisiones de diseno

### 1. Wrapper sobre logue-cli, no acceso directo al hardware

La decision mas importante: **la app no habla con el sintetizador directamente**. En cambio, ejecuta `logue-cli` como proceso externo via `QProcess` y parsea su salida de texto.

```cpp
// LogueCLIWrapper.cpp:17
void LogueCLIWrapper::probe() {
    auto *proc = new QProcess(this);
    connect(proc, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this, proc](int exitCode, QProcess::ExitStatus) {
        auto out = QString::fromUtf8(proc->readAllStandardOutput());
        if (exitCode != 0) {
            emit errorOccurred(...);
        } else {
            emit probeFinished(parseProbeOutput(out));
        }
        proc->deleteLater();
    });
    proc->start(m_cliPath, {QStringLiteral("probe"), QStringLiteral("-l")});
}
```

**Ventajas:**
- No hay necesidad de reverse-engineering del protocolo SysEx
- Compatibilidad con cualquier version futura de logue-cli que respete la interfaz de comandos
- El codigo C++ se enfoca en la logica de la GUI, no en detalles de MIDI de bajo nivel

**Desventaja:**
- Dependencia de que el usuario tenga `logue-cli` instalado separadamente (documentado explicitamente en el README)

### 2. Arquitectura MVC adaptada a Qt/QML

```
src/
├── model/           # Datos puros (UnitInfo, PluginListModel)
├── controller/      # Logica de negocio (AppController, LogueCLIWrapper, UnitHeaderParser)
└── qml/            # Vista declarativa (Main.qml, MidiSection.qml, etc.)
```

- **`UnitInfo`**: plain data struct sin metodos. Solo contiene los campos extraidos del `manifest.json` y un flag `isValid`.
- **`PluginListModel`**: `QAbstractListModel` expuesto a QML con roles custom (`NameRole`, `FilePathRole`, `IsValidRole`).
- **`AppController`**: single source of truth. Expone propiedades y slots a QML via `Q_PROPERTY` y `Q_INVOKABLE`.

### 3. Frontera C++/QML via context property

```cpp
// main.cpp:14
engine.rootContext()->setContextProperty(QStringLiteral("App"), &controller);
```

Un solo objeto `AppController` registrado como `App` en el contexto QML. Toda la comunicacion bidireccional va por ahi:

```cpp
// AppController.h:16-42 (fragmento)
Q_PROPERTY(QString cliPath    READ cliPath    WRITE setCliPath    NOTIFY cliPathChanged)
Q_PROPERTY(QStringList inPorts  READ inPorts  NOTIFY inPortsChanged)
// ...
Q_INVOKABLE void probe();
Q_INVOKABLE void loadUnit();
Q_INVOKABLE void browseCliPath();
```

QML liga directo a las propiedades y llama metodos `Q_INVOKABLE` sin necesidad de bridges adicionales.

### 4. Namespace `qlogue` para evitar contaminar el global namespace

```cpp
namespace qlogue {
class AppController : public QObject { ... };
}
```

Todas las clases del proyecto viven en `qlogue`. `main.cpp` no tiene `using namespace`.

### 5. Parsing de la salida de texto de logue-cli con QRegularExpression

`logue-cli` no provee JSON ni API机器 legible — solo texto plano con formato fijo. Se parsea con expresiones regulares:

```cpp
// LogueCLIWrapper.cpp:48-50
static const QRegularExpression rx(
    R"(^\s*(in|out)\s+(\d+):\s+(.+)$)",
    QRegularExpression::MultilineOption);
```

Un detalle interesting: la herramienta tiene un typo ("ouput" en lugar de "output"). El codigo no lo arregla, simplemente parsea lo que existe.

### 6. Extraccion de manifest.json via `unzip -p` externo

Los archivos `.xxxunit` son ZIPs que contienen un `manifest.json`. Qt no tiene soporte nativo para ZIP, pero `unzip` esta disponible en praticamente todas las distros Linux.

```cpp
// UnitHeaderParser.cpp:20-22
QProcess proc;
proc.setProgram(QStringLiteral("unzip"));
proc.setArguments({QStringLiteral("-p"), filePath, QStringLiteral("*/manifest.json")});
proc.start();

if (!proc.waitForFinished(5000) || proc.exitCode() != 0) {
    info.isValid = false;
    return info;
}
```

Se usa `unzip -p` (pipe) para extraer el contenido directamente a stdout sin escribir archivos temporales. Timeout de 5 segundos.

### 7. Seleccion automatica de puertos MIDI

```cpp
// AppController.cpp:246-252
int AppController::autoSelectPort(const QStringList &list) const
{
    for (int i = 0; i < list.size(); ++i) {
        if (list.at(i).contains(QLatin1String("SOUND"), Qt::CaseInsensitive))
            return i;
    }
    return list.isEmpty() ? -1 : 0;
}
```

Heuristica simple: cualquier puerto que contenga "SOUND" se auto-selecciona (estos son los puertos SysEx correctos para minilogue xd / prologue). Si no hay match, se selecciona el primero. El usuario puede override manual.

### 8. Configuracion persistente con QSettings

```cpp
// AppController.cpp:29-33
QSettings s;
m_cliPath   = s.value(QStringLiteral("tools/logueCli"),
                      QStringLiteral("logue-cli")).toString();
m_pluginDir = s.value(QStringLiteral("library/lastDir")).toString();
```

`QSettings` abstraction la persistencia. Los paths de `logue-cli` y del directorio de plugins se restauran al reiniciar. No hay archivos de configuracion custom — se usa el backend nativo de cada plataforma (Registry en Windows, plist en macOS, ini en Linux).

### 9. CMake con AUTOMOC y AUTORCC

```cmake
# CMakeLists.txt:10-11
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
```

- `AUTOMOC`: Qt genera los archivos `moc_*.cpp` automaticamente desde los headers con `Q_OBJECT`
- `AUTORCC`: el archivo `resources.qrc` se compila automaticamente

Esto elimina la necesidad de archivos `.pro` de Qt Creator o invocar `moc`, `rcc`, `uic` manualmente.

### 10. Opcion de build estatico

```cmake
option(BUILD_STATIC "Link Qt and libc++ statically..." OFF)
if(BUILD_STATIC AND NOT MSVC)
    target_link_options(${PROJECT_NAME} PRIVATE
        -static-libgcc
        -static-libstdc++
    )
endif()
```

Flag opcional `-DBUILD_STATIC=ON` para producir un binario standalone que no dependa de Qt ni libc++ instaladas en el sistema.

### 11. Log de stderr + stdout combinados

```cpp
// LogueCLIWrapper.cpp:72-73
auto combined = QString::fromUtf8(proc->readAllStandardOutput())
              + QString::fromUtf8(proc->readAllStandardError());
```

`logue-cli` mezclaba salida en stdout y stderr, asi que se concatenan antes de parsear y antes de mostrar en el panel de log.

---

## Lecciones aprendidas y posibles mejoras

- **QThread no fue necesario**: toda la E/S es asincrona via signals de QProcess. No hay necesidad de workers en threads separados.
- **QAbstractListModel fue suficiente**: la lista de plugins es plana, no jerarquica. `QAbstractTableModel` o `QTreeView` hubieran sido overkill.
- **QML puro con Controls basicos**: no se usa QML Controls 2 fancy. GroupBox, ListView, SplitView, ComboBox — todos standard. La UI es funcional, no decorated.
- **Falta de tests unitarios**: es un proyecto pequeno pero con logica de parsing no-trivial (regex, JSON). Tests con Qt Test hubieran sido utiles.

---

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/QLogueLibrarian
```

Con build estatico:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC=ON
cmake --build build --parallel
```

---

## Conclusion

QLogueLibrarian demuestra un patron straightforward para envolver herramientas CLI en una GUI Qt/QML: `QProcess` asincrono + signals/slots + `Q_PROPERTY` para binding con QML + `QSettings` para persistencia. La arquitectura MVC adaptada a Qt mantiene la separacion clara entre datos, logica y presentacion. Para un proyecto de esta escala, la complejidad es proporcional y el resultado es mantenible.

El codigo esta disponible en: [https://github.com/david-lafontaine/QLogueLibrarian](https://github.com/david-lafontaine/QLogueLibrarian)
