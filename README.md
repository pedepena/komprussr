# KOMPRUSSR KU-01

Compresor valvular tipo Vari-Mu en formato de plugin de audio (VST3 y AU), con selector de cinco modelos de válvula y una interfaz de estética soviética de los años 50 y 60: chapa roja desgastada, knobs chicken head de baquelita y medidor de reducción de ganancia (GR).

Es gratuito y de código abierto.

> **Estado:** el plugin está terminado y listo para su primera versión pública. Se ha probado en FL Studio. Las descargas están en la sección [Releases](../../releases).
<!-- CONFIRMAR antes de publicar: que la release exista y que los nombres de archivo de la sección "Descarga e instalación" coincidan con los reales. -->

## Descarga e instalación

Descarga el archivo de tu sistema desde [Releases](../../releases). Hay dos formas de instalarlo en Windows: con instalador o a mano.

### Windows

**Con instalador (recomendado):**
1. Descarga el instalador (`.exe`) y haz doble clic.
2. Si Windows muestra el aviso azul «Windows protegió su equipo», pulsa **Más información** y después **Ejecutar de todas formas**. Aparece porque el instalador aún no lleva firma digital; no significa que haya ningún problema.
3. Sigue los pasos hasta terminar.
4. Abre tu programa de música y busca nuevos plugins. En FL Studio: *Options > Manage plugins > Find plugins*.

**A mano (sin instalador):**
1. Descarga el `.zip` y descomprímelo (clic derecho, «Extraer todo»).
2. Copia la carpeta `KOMPRUSSR KU-01.vst3` a `C:\Program Files\Common Files\VST3`. Windows pedirá permiso de administrador.
3. Abre tu programa de música y busca nuevos plugins.

### macOS

1. Descarga el instalador (`.pkg`) y haz doble clic.
2. Si macOS dice que no se puede abrir por ser de un «desarrollador no identificado», haz clic derecho sobre el archivo y elige **Abrir**. Si no aparece esa opción, ve a *Ajustes del Sistema > Privacidad y seguridad* y pulsa **Abrir igualmente**.
3. Sigue los pasos hasta terminar. Se instalan VST3 y AU.
4. Abre tu programa de música. Logic detecta el plugin solo; si no aparece, reinícialo.

### Linux

1. Descarga el archivo comprimido y descomprímelo.
2. Copia la carpeta `KOMPRUSSR KU-01.vst3` a `~/.vst3`.
3. Abre tu programa de música y busca nuevos plugins.

### Compatibilidad

| Sistema | Formatos |
|---|---|
| Windows | VST3 |
| macOS | VST3 y AU |
| Linux | VST3 |

Funciona en pistas mono y estéreo. Se ha probado en FL Studio; debería funcionar en cualquier programa que admita VST3 (o AU en Mac), pero todavía no se ha comprobado en todos. Si encuentras un problema, abre una [incidencia](../../issues) indicando tu sistema y tu programa de música.
<!-- CONFIRMAR: macOS (VST3 y AU) y Linux solo deben aparecer aquí cuando esas versiones estén compiladas y probadas. -->

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

El resultado es el plugin de KOMPRUSSR KU-01 en la carpeta `build`. La copia automática a la carpeta de plugins está desactivada.
<!-- CONFIRMAR: cuando CMakeLists.txt incluya el formato AU, indicar aquí que en macOS también se genera el AU. -->

## Estructura del repositorio

```
Source/          código del plugin (C++ / JUCE): procesador y editor
CMakeLists.txt   proyecto de CMake
assets/ui/       capas gráficas en PNG, manifest.json y vistas previas
docs/            manual de usuario, especificación de la interfaz, guía de válvulas y notas de DSP
tools/           utilidades de desarrollo (generación de arañazos del cristal)
AGENTS.md        resumen técnico del proyecto para asistentes de programación
CHANGELOG.md     historial de cambios por versión
```

## Límites conocidos

- Los modelos de válvula son aproximaciones perceptuales. Sirven para dar carácter, no para reproducir un equipo concreto.
- Los valores se han ajustado por escucha. Aún no hay mediciones formales publicadas (distorsión, curvas de reducción).
- Con Power OFF el audio pasa sin procesar y sin el pequeño retardo que el plugin declara al programa de música, por lo que al alternar ON/OFF puede haber un desfase mínimo. Está anotado como mejora pendiente.

## Licencia y créditos

El código se publica bajo la licencia **GNU AGPL v3** (ver `LICENSE`), porque usa el framework [JUCE](https://juce.com) bajo sus términos de código abierto (AGPLv3). Salvo que se indique otra cosa, las imágenes de `assets/ui` se publican con la misma licencia.

Hecho con [JUCE](https://juce.com).

KOMPRUSSR es un proyecto independiente y no está afiliado a ningún fabricante de equipos ni de válvulas. Los nombres de las válvulas se usan solo para identificar el carácter en que se inspira cada modelo.
