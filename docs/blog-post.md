# QLogueLibrarian: un wrapper GUI multiplataforma para KORG logue-cli

**Autor:** David Emmanuel Lopez  
**Stack:** C++17 / Qt6 / QtQuick QML / CMake  
**Licencia:** GPL v3

---

## Introduccion

`QLogueLibrarian` es una interfaz grafica minimalista que envuelve la herramienta oficial `logue-cli` de KORG para subir unidades (osciladores, efectos) a sintetizadores de la serie logue (minilogue xd, prologue, NTS-1, drumlogue, etc.). El problema original: no existe un equivalente de Korg Sound Librarian para Linux, y los drivers USB-MIDI de Korg son problematicos en Windows. En Linux los dispositivos logue son reconocidos out-of-the-box como dispositivos ALSA MIDI standard.

El codigo fuente esta disponible en [GitHub](https://github.com/david-lafontaine/QLogueLibrarian).

---

## La cuestión de fondo: ¿qué variante de MVC?

"Usamos MVC" no dice casi nada: bajo la misma sigla conviven el MVC clásico
(Smalltalk), el MVP y variantes que solo comparten la separación
Modelo/Vista/Controlador. La pregunta que realmente importa en Qt/QML es:
**¿qué debe hacer el "controller" y qué puede leer/escribir la vista?** Las
opciones concretas que se evaluaron:

- **A. Controller mediador total (MVP / *passive view*)**: el controller
  oculta el modelo; la vista es tonta y pide todo por métodos. Es la lectura
  más común del diagrama "View → Controller → Model".
- **B. MVC clásico**: la vista lee el modelo directamente (Observer) y el
  controller traduce la *intención del usuario* en mutaciones. El controller
  no media lecturas; nadie conoce al controller.
- **C. MVVM/ViewModel**: un objeto expone estado observable + comandos a los
  que la vista se suscribe.
- **D. God object**: un único `QObject` con estado, propiedades, orquestación
  y hasta textos de UI. Fue el punto de partida real del proyecto y lo que se
  corrigió.

### Por qué se descartó A (y por qué en QML A degenera en C)

La vista QML no "pinta por código": son **bindings declarativos**
(`property X: App.algo`) reactivos a `NOTIFY`. Para que la vista fuera tonta,
el controller debería re-exponer cada pedazo de estado como `Q_PROPERTY` — es
decir, **duplicar el modelo** y convertirse en un ViewModel con otro nombre.
Además, cada acceso cruza la frontera C++/QML vía `QVariant`; leer directo de
un objeto tipado es más simple y barato que hacer round-trips por un
intermediario. El binding declarativo ya cumple el rol del Observer: no hace
falta que nadie le "empuje" la vista.

### Por qué la respuesta final está entre B y C, con matices

Qt ya separa por mecanismo nativo lo que B separa conceptualmente: **estado
observable** (`Q_PROPERTY` + `NOTIFY`) por un lado y **acciones**
(`Q_INVOKABLE`/slots) por el otro. En Qt el "controller" no media lecturas:
media la intención (probe, escanear, subir). El resultado concreto del
proyecto fue:

1. **Modelo observable y de solo lectura para QML.** `Logic` (ex `AppState`)
   expone 7 `Q_PROPERTY` **sin `WRITE`** (`cliPath`, `unitDir`, `inPorts`,
   `outPorts`, `library`, `statusText`, `logText`). La vista bindea para
   mostrar; no puede modificar el modelo.
2. **Un solo escritor.** El store y el orquestador se fusionaron en un único
   objeto (`Logic`): el que posee el estado es el que lo muta
   (`probe`, `scanUnits`, `loadUnit`, `setCliPath`, `setUnitDir`). Así
   desaparece la ambigüedad de "¿quién escribe el modelo?" que existía cuando
   el estado y la orquestación vivían en dos QObjects distintos.
3. **Selección y presentación → la vista.** `ViewState` guarda qué unit/puerto
   está elegido, el target de carga y el slot. El modelo no guarda selección.
4. **`AppController` como gateway delgado.** Es el *único camino* por el que
   la vista comanda: valida parámetros, delega en `Logic` y traduce las
   señales de operación de `Logic` (`probeFinished`, `scanFinished`,
   `loadFinished`, `errorOccurred`) a los textos de `statusText`/`logText`.
   Las cadenas de usuario viven acá, no en la lógica.
5. **Tipos encapsulados como value types.** Los metadatos de una unit viajan
   como `UnitInfo`/`UnitParam` (`Q_GADGET` registrados en `main.cpp`): una
   unit entra y sale del QML como un solo valor tipado, no como una decena de
   propiedades sueltas (`metaName`, `metaPlatform`, ...) que duplicaban datos.
6. **Separación física por capa.** `model/` (tipos puros), `logic/` (`Logic` +
   `LogueCLIWrapper` + `UnitHeaderParser`), `controller/` (solo
   `AppController`) y `view/` (`ViewState`, `Dialogs`). `App` apunta a
   `Logic`; `Controller` a `AppController`.

### La decisión, en una frase

La arquitectura final es un híbrido consciente: **MVC clásico en la dirección
de lectura** (la vista lee el modelo; el binding reemplaza al Observer),
**command gateway en la dirección de escritura** (`AppController` como único
camino QML→modelo) y **presentación separada del dominio** (selección en
`ViewState`, textos de UI en el gateway). Es la variante que menos pelea con
el paradigma declarativo de QML y la que preserva la regla crítica del
proyecto: *el modelo no se puede modificar desde la vista*.

---

## Decisiones de diseno

> Las secciones numeradas siguientes documentan decisiones tomadas durante el
> desarrollo e incluyen fragmentos del diseño inicial (antes del refactor de
> arquitectura). El estado final de la separación Modelo/Controlador/Vista es
> el que describe la sección anterior ("¿qué variante de MVC?"); donde haya
> conflicto, manda esa sección.

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

### 2. Arquitectura final por capas

```
src/
├── model/       # Tipos puros: UnitInfo/UnitParam (value types Q_GADGET), MidiPort, KorgEnums
├── logic/       # Logic (modelo observable + único orquestador), LogueCLIWrapper, UnitHeaderParser
├── controller/  # AppController (gateway QML delgado)
├── view/        # ViewState (selección), Dialogs
└── qml/         # Vista declarativa (Main.qml, UnitLibrary.qml, MetaPanel.qml, ...)
```

- **`Logic`** (ex `AppState`): estado observable read-only para QML (7
  `Q_PROPERTY` sin `WRITE`) **y** orquestador único de negocio (probe/scan/
  load/settings). Se muta a sí mismo: no hay ambigüedad sobre quién escribe el
  modelo. Posee `LogueCLIWrapper` y emite señales de operación
  (`probeFinished`, `scanFinished`, `loadFinished`, `errorOccurred`).
- **`UnitInfo`/`UnitParam`**: value types (`Q_GADGET`) registrados en QML.
- **`AppController`**: único camino QML→modelo. Valida, delega en `Logic` y
  traduce sus señales de operación a `statusText`/`logText`.
- **`ViewState`**: selección y valores de formulario (qué unit/puerto está
  elegido, slot, target de carga). Es estado de la vista, no del modelo.

### 3. Frontera C++/QML via context properties

```cpp
// main.cpp
engine.rootContext()->setContextProperty("App",        &logic);
engine.rootContext()->setContextProperty("Controller", &controller);
engine.rootContext()->setContextProperty("ViewState",  &viewState);
engine.rootContext()->setContextProperty("Dialogs",    &dialogs);
```

Hay **cuatro** objetos de contexto con roles distintos: `App` es el modelo
(read-only), `Controller` es el gateway de comandos, `ViewState` es la
selección de la vista y `Dialogs` es el helper de diálogos. QML liga a las
propiedades read-only de `App` y llama a los `Q_INVOKABLE` de `Controller`; no
puede modificar el modelo directamente.

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
