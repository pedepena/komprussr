# KOMPRUSSR KU-01 — resumen técnico para asistentes de programación

Plugin de audio VST3/AU en C++ con JUCE: compresor valvular tipo Vari-Mu. Proyecto personal, de código abierto (AGPL-3.0), distribuido gratis por GitHub.

## Estado actual
Las fases 1 (interfaz y parámetros) y 2 (motor de audio) están terminadas y probadas en FL Studio: guardado y restauración del proyecto, volumen igualado entre válvulas y retoques estéticos cerrados. Lo que queda es la **fase 3: distribución** (ver más abajo). No hay trabajo pendiente de diseño ni de sonido salvo lo que pida el responsable.

## Reglas de trabajo
- Antes de modificar código, inspecciona el proyecto y di qué archivos vas a tocar. No inventes clases, archivos ni APIs: adáptate al código real.
- Código de audio seguro para tiempo real: sin asignaciones de memoria, bloqueos ni operaciones pesadas en el hilo de audio.
- Sin controles extra de sonido ni función A/B. El engranaje, las pestañas About/Manual y la casilla de comprobación de actualizaciones dentro de About son solo elementos de interfaz: no son controles de sonido ni parámetros, no se automatizan ni se guardan en el estado del proyecto. La preferencia y la fecha de consulta viven en un archivo de ajustes del usuario. La comprobación automática de versiones es la única conexión automática a internet.
- No afirmes que hay emulación física exacta de ningún equipo: son modelos inspirados en las válvulas.
- No añadas etapas de señal nuevas, topologías ni combinaciones de válvulas. Los filtros y las curvas internas de cada perfil de válvula (`ValveModel`) sí están permitidos, siempre que no añadan controles ni cambien la interfaz.
- Explica los cambios en lenguaje sencillo: el responsable del proyecto no es programador.
- No subas al repositorio binarios de `build/` ni la carpeta `JUCE/`.

## Parámetros (AudioProcessorValueTreeState, automatizables y guardados en el estado)
| ID | Rango | Notas |
|---|---|---|
| `compression` | 0 a 100 % | por defecto 0 % (decidido) |
| `make_up_gain` | -12 a +24 dB | por defecto 0 dB |
| `tube` | 5 pasos | 6386U (por defecto), 6BA6, 6BC8, 6K7, 6SK7 |
| `power` | ON/OFF | por defecto ON |

La reducción de ganancia (`gainReductionDb`) es un valor interno de solo lectura que alimenta el medidor; no es un parámetro del host.

Cambiar de válvula no resetea el resto de ajustes. Evitar saltos de volumen al cambiar de modelo.

## Interfaz
- Lienzo de 1376 × 768 px, escalable de forma uniforme. Todo en PNG; **no se usan filtros SVG**.
- `assets/ui/manifest.json` define posición, tamaño, pivote, rangos de giro, orden de apilado, estados y la calibración de la aguja. `assets/ui/LEEME.txt` lo resume. La interfaz está integrada en `Source/PluginEditor.*`; las imágenes se incrustan desde CMake.
- Los knobs giran alrededor del centro de su imagen (-135° a +135°; el selector Tube usa -120°, -60°, 0°, +60°, +120°). Los PNG de los tres knobs son los regenerados: acabado mate y sin iluminación horneada (CMake los incrusta y `PluginEditor.cpp` los carga). Las sombras y los reflejos están en capas separadas que se dibujan fijas, incluida `knob_highlights_overlay`, según el manifest; por eso no giran con el mando.
- Make Up Gain a 0 dB corresponde a -45°.
- La aguja del medidor usa la tabla `calibracion_db_a_grados` (interpolación lineal) y se dibuja recortada al círculo y a la ventana indicados en el manifest. En reposo marca 0 dB.
- El panel base no contiene el texto GR ni la estrella. `gr_label.png` contiene el texto extraído, movido 12 px arriba y centrado en x=699; la estrella es una forma `juce::Path` de 5 puntas, color `#7A160F`, centrada en (699, 290.5), 36 × 36 px. Ambas capas quedan debajo de la aguja y del cristal.
- ON: `tumbler_on` visible, válvulas y piloto con fundido de opacidad a 1. OFF: `tumbler_off`, válvulas y piloto con fundido a 0, aguja en 0 dB. La rejilla permanece fija.
- El engranaje vectorial abre una capa con las pestañas About y Manual. About incluye la opción de comprobación automática de versiones, guardada en el archivo de ajustes del usuario y no en el proyecto.
- La ventana inicia en 1032 × 576 px, restaura el último tamaño del estado APVTS y mantiene la proporción del lienzo al redimensionar.
- El cristal lleva rayaduras y roces sutiles generados con semilla fija por `tools/generate_glass_scratches.js`; se incrustan las versiones 1x y @2x. La copia intacta está en `assets/ui/originales/vu_glass_overlay.png`.

## Válvulas
Cada modelo cambia de forma coherente la curva de control de ganancia, la rodilla (knee), la respuesta a transitorios, la recuperación y la generación de armónicos (dependiente del nivel y de la reducción), sin ser una EQ estática. Todos deben ser estables y utilizables. Parámetros y tablas actuales en `docs/modelos_dsp.md`.

`docs/guia_valvulas_vari_mu.pdf` es la guía original. Donde difiera de este archivo, mandan las decisiones posteriores: el selector tiene **cinco** válvulas (sin 6BZ8, sin 5670, sin 6L7), la 6386 se rotula **6386U** y **no hay función A/B**.

## Decisiones tomadas (antes eran puntos abiertos)
- Compression arranca en 0 %.
- Calentamiento visual de las válvulas: 1,5 s.
- Knobs y selector siguen utilizables en OFF.
- Suavizado de la aguja: unos 300 ms.
- Son decisiones de presentación o de valores iniciales; las de aspecto no cambian el audio.

## Mejoras menores pendientes (no bloquean la publicación)
- **Latencia con Power OFF:** el plugin declara una latencia fija (sobremuestreo), pero en OFF devuelve el audio sin procesar y sin ese retardo. La compensación del host no corrige ese caso, así que al alternar ON/OFF puede haber un desfase mínimo, sin medir. Mejora posible: retrasar la señal sin procesar en OFF lo mismo que declara el plugin. Solo audio; requiere avisar antes de tocar `PluginProcessor`.
- Mediciones formales (distorsión y curvas de reducción) para publicarlas; `docs/modelos_dsp.md` explica el procedimiento.

## Fase 3: distribución (pendiente)
Objetivo: que cualquier persona pueda descargar e instalar el plugin en Windows, macOS y Linux, y usarlo en cualquier programa de música compatible.

- **Formatos:** VST3 en los tres sistemas; AU además en macOS. `CMakeLists.txt` solo genera VST3 hoy: hay que añadir `AU` a `FORMATS` para la versión de Mac. Pro Tools (AAX) queda fuera: exige un acuerdo con Avid que no encaja con un plugin gratuito.
- **Compilación:** automática en GitHub Actions, una máquina por sistema (Windows, macOS, Linux), para no depender de tener un Mac o un Linux. Los dos sistemas necesitan probarse con escucha real por una persona antes de publicar.
- **Entregables:** Windows: instalador `.exe` y `.zip` con la carpeta `.vst3` (para quien desconfíe de los ejecutables). macOS: instalador `.pkg` con VST3 y AU. Linux: archivo comprimido con la carpeta `.vst3`. Los pasos de instalación para el usuario están en `README.md`.
- **Firma de Windows:** se solicitará SignPath Foundation (firma gratuita para código abierto: exige licencia OSI y repositorio público, y una página pública de política de firma). Hasta que se apruebe, el instalador mostrará el aviso de Windows y el README lo explica. La firma y la notarización de macOS requieren una cuenta de pago de Apple; de momento no se hacen.
- **Versión:** la primera versión pública será la 1.0.0. `CMakeLists.txt` aún indica 0.1.0.
- **Notas de la release:** versión, sistemas y programas de música probados, tipo y ubicación de cada instalador, estado de la firma del `.exe` y una nota clara sobre el aviso SmartScreen mientras no esté firmado.
- **Referencia:** el responsable tiene el repositorio de otro plugin suyo donde ya se hicieron instaladores. Puede servir de base, pero hay que revisarlo y optimizarlo antes de reutilizarlo.

## Documentación
- `README.md`: portada del proyecto y guía de instalación para usuarios.
- `CHANGELOG.md`: historial de cambios por versión.
- `docs/manual_usuario.md`: manual de usuario.
- `docs/modelos_dsp.md`: parámetros y notas del motor de audio, y procedimiento de comprobación.
- `docs/KOMPRUSSR_KU-01_especificacion_UI.pdf`: especificación completa de la interfaz.
- `docs/guia_valvulas_vari_mu.pdf`: guía original de las válvulas (ver nota en la sección Válvulas).
- `docs/estado_plugin_para_claude.pdf`: informe del 11 de noviembre de 2026; es histórico y ya no refleja el estado actual (por ejemplo, indica Compression por defecto al 50 % y una interfaz provisional).
