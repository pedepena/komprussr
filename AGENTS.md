# KOMPRUSSR KU-01 — resumen técnico para asistentes de programación

Plugin de audio VST3/AU en C++ con JUCE: compresor valvular tipo Vari-Mu. Proyecto personal, de código abierto (AGPL-3.0), distribuido gratis por GitHub.

## Reglas de trabajo
- Antes de modificar código, inspecciona el proyecto y di qué archivos vas a tocar. No inventes clases, archivos ni APIs: adáptate al código real.
- Código de audio seguro para tiempo real: sin asignaciones de memoria, bloqueos ni operaciones pesadas en el hilo de audio.
- Sin controles extra ni función A/B en esta fase. No añadas etapas, topologías ni combinaciones de válvulas.
- No afirmes que hay emulación física exacta de ningún equipo: son modelos inspirados en las válvulas.
- Todo se hace por fases: (1) interfaz y parámetros con el audio sin procesar, (2) motor de audio.

## Parámetros (AudioProcessorValueTreeState, automatizables y guardados en el estado)
| ID | Rango | Notas |
|---|---|---|
| `compression` | 0 a 100 % | valor por defecto sin decidir |
| `make_up_gain` | -12 a +24 dB | por defecto 0 dB |
| `tube` | 5 pasos | 6386U, 6BA6, 6BC8, 6K7, 6SK7 |
| `power` | ON/OFF | |
| `gain_reduction` | dB (solo lectura, para el medidor) | |

Cambiar de válvula no resetea el resto de ajustes. Evitar saltos de volumen al cambiar de modelo.

## Interfaz
- Lienzo de 1376 × 768 px, escalable de forma uniforme. Todo en PNG; **no se usan filtros SVG**.
- `assets/ui/manifest.json` define posición, tamaño, pivote, rangos de giro, orden de apilado, estados y la calibración de la aguja. `assets/ui/LEEME.txt` lo resume. La interfaz final está integrada en `Source/PluginEditor.*`; las imágenes se incrustan desde CMake.
- Los knobs giran alrededor del centro de su imagen (-135° a +135°; el selector Tube usa -120°, -60°, 0°, +60°, +120°). Las sombras permanecen fijas y los reflejos de cada mando giran con él.
- Make Up Gain a 0 dB corresponde a -45°.
- La aguja del medidor usa la tabla `calibracion_db_a_grados` (interpolación lineal) y se dibuja recortada al círculo y a la ventana indicados en el manifest. En reposo marca 0 dB.
- ON: tumbler_on visible, válvulas y piloto con fundido de opacidad a 1. OFF: tumbler_off, válvulas y piloto con fundido a 0, aguja en 0 dB. El brillo de las válvulas se separa visualmente de la rejilla, que permanece fija y sin cambios de color.
- La ventana inicia en 1032 × 576 px, restaura el último tamaño del estado APVTS y mantiene la proporción del lienzo al redimensionar. Los reflejos de Compression, Make Up Gain y Tube giran con sus mandos; el reflejo del tumbler queda fijo.
- Verificación pendiente en un host: orientación visual de los knobs y aguja y respuesta de interacción.

## Válvulas (fase 2)
Cada modelo debe cambiar de forma coherente la curva de control de ganancia, la rodilla (knee), la respuesta a transitorios, la recuperación y la generación de armónicos (dependiente del nivel y de la reducción), sin ser una EQ estática. Todos deben ser estables y utilizables. Detalle en `docs/guia_valvulas_vari_mu.pdf`.

## Puntos sin decidir
Compression conserva el valor por defecto definido por el procesador (0 %); no se ha cambiado. Decisiones adoptadas para la interfaz: calentamiento visual de 1,5 s; knobs y selector utilizables en OFF; suavizado de aguja de unos 300 ms. Estas decisiones son solo de presentación y no cambian el audio.

## Documentación
`docs/KOMPRUSSR_KU-01_especificacion_UI.pdf` es la especificación completa de la interfaz.
