# KOMPRUSSR KU-01

Compresor valvular tipo Vari-Mu en formato de plugin de audio (VST3 y AU), con selector de cinco modelos de válvula y una interfaz de estética soviética de los años 50 y 60: chapa roja desgastada, knobs chicken head de baquelita y medidor de reducción de ganancia (GR).

Es gratuito y de código abierto. Los paquetes de Windows, macOS y Linux se generan con GitHub Actions; el instalador de Windows se distribuye sin firma digital.

> **Estado:** el plugin está terminado y listo para su primera versión pública. Se ha probado en FL Studio. Las descargas están en la sección [Releases](../../releases).

## Descarga e instalación

Descarga el archivo de tu sistema desde [Releases](../../releases). Cada release incluye un instalador y un paquete portátil cuando se indica a continuación.

### Windows

**Con instalador (recomendado):**
1. Descarga el instalador (`.exe`) y haz doble clic.
2. El instalador no está firmado. Windows puede mostrar SmartScreen porque todavía no reconoce su reputación. Comprueba que lo descargaste desde este repositorio antes de elegir **Más información > Ejecutar de todas formas**.
3. Sigue los pasos hasta terminar.
4. Abre tu programa de música y busca nuevos plugins. En FL Studio: *Options > Manage plugins > Find plugins*.

**A mano (sin instalador):**
1. Descarga el `.zip` y descomprímelo (clic derecho, «Extraer todo»).
2. Copia la carpeta `Komprussr KU-01.vst3` a `C:\Program Files\Common Files\VST3`. Windows pedirá permiso de administrador.
3. Abre tu programa de música y busca nuevos plugins.

### macOS

**Con instalador:** descarga el `.pkg`, haz doble clic y sigue los pasos. Instala VST3 y AU.

**Paquete portátil:** descomprime el ZIP. Copia `VST3/Komprussr KU-01.vst3` a `~/Library/Audio/Plug-Ins/VST3/` y, si quieres AU, copia `AU/Komprussr KU-01.component` a `~/Library/Audio/Plug-Ins/Components/`. Crea las carpetas si no existen. Logic usa AU; otras aplicaciones pueden usar VST3.

Los paquetes no están firmados ni notarizados; macOS puede mostrar un aviso de seguridad al instalarlos o abrirlos.

### Linux

1. Descarga y descomprime el `.tar.gz`.
2. Para instalar en tu usuario, abre una terminal en la carpeta extraída y ejecuta `sh install.sh`. También puedes copiar `Komprussr KU-01.vst3` a `~/.vst3` manualmente.
3. Abre tu programa de música y vuelve a escanear los plugins. El script no necesita permisos de administrador.

### Compatibilidad

| Sistema | Formatos |
|---|---|
| Windows | VST3 |
| macOS | VST3 y AU |
| Linux | VST3 |

Funciona en pistas mono y estéreo. Se ha probado en FL Studio; debería funcionar en cualquier programa que admita VST3 (o AU en Mac), pero todavía no se ha comprobado en todos. Si encuentras un problema, abre una [incidencia](../../issues) indicando tu sistema y tu programa de música.

## Download and installation (English)

Download your platform's files from [Releases](../../releases). Each release provides the installer and portable package described below.

### Windows

**Installer:** Run the `.exe` and follow the prompts. It installs the VST3 in the system plug-in folder. The installer is unsigned, so Windows may show a SmartScreen warning. Verify that the file came from this repository before choosing **More info > Run anyway**.

**Portable ZIP:** Extract the archive and copy `Komprussr KU-01.vst3` to `C:\Program Files\Common Files\VST3`. Windows may ask for administrator permission. Rescan plug-ins in your music software.

### macOS

**Installer:** Open the `.pkg` and follow the prompts. It installs both VST3 and AU.

**Portable ZIP:** Extract the archive. Copy `VST3/Komprussr KU-01.vst3` to `~/Library/Audio/Plug-Ins/VST3/`. To install AU, copy `AU/Komprussr KU-01.component` to `~/Library/Audio/Plug-Ins/Components/`. Create the folders if needed. Logic uses AU; other applications may use VST3.

The packages are not signed or notarized; macOS may show a security warning during installation or when opening them. If blocked, control-click the package and choose **Open**, or use **System Settings > Privacy & Security > Open Anyway** when offered.

### Linux

Extract the `.tar.gz` archive. To install for your user, open a terminal in the extracted folder and run `sh install.sh`. You can also copy `Komprussr KU-01.vst3` to `~/.vst3` manually. Rescan plug-ins in your music software. The script does not need administrator permissions.

## Controles

- **Compression:** 0 a 100 % (por defecto 0 %).
- **Make Up Gain:** de -12 a +24 dB (0 dB por defecto).
- **Tube:** 5 modelos de válvula: 6386U (por defecto), 6BA6, 6BC8, 6K7 y 6SK7. Cada uno cambia el umbral, la relación de compresión, la rodilla (knee), los tiempos de ataque y recuperación y el color armónico. Son modelos *inspirados en* esas válvulas; no son emulaciones exactas de ningún equipo concreto.
- **Power:** ON/OFF. En OFF el audio pasa sin procesar y se apagan el piloto y las válvulas.
- **Medidor GR:** aguja de reducción de ganancia. Reposa en 0 y baja hacia -20 al comprimir.

Los parámetros son automatizables y se guardan con el proyecto. Cambiar de válvula no modifica el resto de ajustes. Hay un [manual de usuario](docs/manual_usuario.md) en español.

## Ajustes y privacidad

Un engranaje en la esquina inferior derecha abre las pestañas **About** y **Manual**. En **About** puedes activar o desactivar la comprobación automática de nuevas versiones.

Por defecto, el plugin consulta `github.com` para saber si hay una versión nueva. Es la única conexión a internet que hace, y se puede desactivar. La preferencia se guarda en un archivo de ajustes de tu usuario, no en el proyecto.

## Cómo funciona por dentro

- Detector de envolvente con ataque y recuperación propios de cada válvula; la recuperación varía con el nivel de la señal.
- Curva de compresión con rodilla suave.
- Saturación con términos pares e impares que depende del nivel y de la reducción real, con sobremuestreo x2, bloqueo de continua y filtro de tono por modelo.
- Fundido corto al cambiar de válvula para evitar saltos de volumen.
- El plugin informa al host de la latencia del sobremuestreo.

Los valores de cada modelo son decisiones creativas, no mediciones de circuitos reales. Más detalle y un procedimiento de comprobación en [`docs/modelos_dsp.md`](docs/modelos_dsp.md).

## Interfaz

- Lienzo de 1376 × 768 px, con tamaño inicial de 1032 × 576 px y proporción fija. El último tamaño se guarda en el estado del proyecto.
- Capas PNG incrustadas en el plugin y dibujadas según el orden y las coordenadas de `assets/ui/manifest.json`.
- Los fundidos de encendido y apagado de las válvulas y del piloto duran 1,5 s. La aguja sigue la reducción de ganancia con un suavizado de unos 300 ms.
- Los arañazos del cristal del medidor se generan con semilla fija mediante `tools/generate_glass_scratches.js`.

La especificación completa de la interfaz está en `docs/KOMPRUSSR_KU-01_especificacion_UI.pdf`.

## Compilar

Requisitos: CMake 3.22 o superior, un compilador con C++17 y [JUCE](https://juce.com) (se ha usado la versión 9.0.5).

1. Descarga JUCE y colócalo en una carpeta `JUCE/` en la raíz del repositorio (está en `.gitignore`).
2. Configura y compila:

```
cmake -B build
cmake --build build --config Release
```

El resultado es el plugin de KOMPRUSSR KU-01 en la carpeta `build`. CMake genera VST3 en Windows, macOS y Linux, y AU además en macOS. La copia automática a la carpeta de plugins está desactivada.

## Estructura del repositorio

```
Source/          código del plugin (C++ / JUCE): procesador y editor
CMakeLists.txt   proyecto de CMake
assets/ui/       capas gráficas en PNG, manifest.json y vistas previas
docs/            manual de usuario, especificación de la interfaz, guía de válvulas y notas de DSP
tools/           utilidades de desarrollo (generación de arañazos del cristal)
AGENTS.md        resumen técnico del proyecto para asistentes de programación
docs/CHANGELOG.md historial de cambios por versión
```

## Límites conocidos

- Los modelos de válvula son aproximaciones perceptuales. Sirven para dar carácter, no para reproducir un equipo concreto.
- Los valores se han ajustado por escucha. Aún no hay mediciones formales publicadas (distorsión, curvas de reducción).
- Con Power OFF el audio pasa sin procesar y sin el pequeño retardo que el plugin declara al programa de música, por lo que al alternar ON/OFF puede haber un desfase mínimo. Está anotado como mejora pendiente.

## Licencia y créditos

El código se publica bajo la licencia **GNU AGPL v3** (ver `LICENSE`), porque usa el framework [JUCE](https://juce.com) bajo sus términos de código abierto (AGPLv3). Salvo que se indique otra cosa, las imágenes de `assets/ui` se publican con la misma licencia.

Hecho con [JUCE](https://juce.com).

KOMPRUSSR es un proyecto independiente y no está afiliado a ningún fabricante de equipos ni de válvulas. Los nombres de las válvulas se usan solo para identificar el carácter en que se inspira cada modelo.
