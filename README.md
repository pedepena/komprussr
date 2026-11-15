# KOMPRUSSR KU-01

Compresor valvular tipo Vari-Mu en formato de plugin de audio (VST3), con selector de modelos de válvula y una interfaz de estética soviética de los años 50 y 60: chapa roja desgastada, knobs chicken head de baquelita y medidor de reducción de ganancia (GR).

> **Estado:** en desarrollo. El motor de audio tiene una primera versión funcional y la interfaz gráfica con capas PNG ya está integrada. La interfaz compila como VST3 Release; falta verificar su aspecto y controles dentro de un host de audio.

## Qué funciona hoy

- **Compression:** 0 a 100 % (por defecto 0 %).
- **Make Up Gain:** de -12 a +24 dB (0 dB por defecto).
- **Tube:** 5 modelos de válvula: 6386U (por defecto), 6BA6, 6BC8, 6K7 y 6SK7. Cada uno cambia el umbral, el ratio, el knee, los tiempos de ataque y recuperación y el color armónico. Son modelos *inspirados en* esas válvulas; no son emulaciones exactas de ningún equipo concreto.
- **Power:** ON/OFF. En OFF el audio pasa sin procesar.
- **Medidor GR:** el motor calcula la reducción de ganancia; la interfaz provisional la muestra en texto.
- Los parámetros son automatizables y se guardan con el estado del host.

### Motor de audio (primera aproximación)

- Detector de envolvente con ataque y recuperación dependientes de la válvula, y recuperación que varía con el nivel de la señal.
- Curva de compresión con knee suave.
- Saturación con términos pares e impares que depende de la cantidad de compresión, con sobremuestreo x2, bloqueo de continua y filtro de tono por modelo.
- Fundido corto al cambiar de válvula para evitar saltos de volumen.
- El plugin informa al host de la latencia del sobremuestreo.

Los valores de cada modelo son decisiones creativas iniciales, no mediciones de circuitos reales. Más detalle y un procedimiento de comprobación en [`docs/modelos_dsp.md`](docs/modelos_dsp.md).

## Interfaz

- Lienzo de 1376 × 768 px, con tamaño inicial de 1032 × 576 px, proporción fija y último tamaño redimensionado guardado en el estado del proyecto.
- Capas PNG incrustadas en el VST3 y dibujadas según el orden y las coordenadas de `assets/ui/manifest.json`. Las capas móviles y el cristal del medidor usan sus imágenes @2x; el panel base usa su imagen 1x. El cristal lleva arañazos y roces sutiles generados con semilla fija por `tools/generate_glass_scratches.js`.
- Knobs de Compression y Make Up Gain, selector de cinco válvulas, interruptor ON/OFF y medidor GR calibrado. El valor de 0 dB de Make Up Gain sitúa el mando en -45°.
- El texto GR se conserva en una capa transparente extraída del arte original, centrada en el eje vertical de la aguja y subida 12 px. La estrella se dibuja como forma vectorial roja de 36 px, encima del texto y debajo de la aguja y el cristal.
- El piloto y el brillo de las válvulas hacen fundidos de 1,5 s al encender y apagar. La rejilla permanece fija. La capa `knob_highlights_overlay` ya queda fija; los reflejos que vienen pintados dentro de los PNG de los knobs todavía giran con ellos y requieren regenerar esas imágenes para desaparecer. La aguja sigue la reducción de ganancia con suavizado de unos 300 ms.
- Los controles están conectados a los parámetros guardables y automatizables existentes. No se modificó el motor de audio.
- Un engranaje abre las pestañas **About**, **Manual** y **Settings**. Settings permite activar o desactivar la comprobación automática de nuevas versiones.
- Por defecto consulta `github.com` para buscar actualizaciones; se puede desactivar en Settings. La preferencia se guarda en los ajustes del usuario, fuera del estado del proyecto.
- El manual de usuario está disponible en [español](docs/manual_usuario.md).
- Pendiente: regenerar los PNG de knobs sin reflejos integrados antes de fijar su brillo; abrir el VST3 en un host y revisar visualmente orientación y controles.

## Pendiente

- Verificar visualmente en un host que los extremos de giro y la marca de reposo de la aguja coinciden con el arte; la compilación por sí sola no confirma la orientación visual.
- Regenerar los PNG de Compression, Make Up Gain y Tube sin brillos integrados para poder dibujar los reflejos como una capa fija independiente.
- Confirmar el valor por defecto de Compression con el responsable del proyecto.
- Formato AU (ahora solo se compila VST3).
- Probar en macOS y Linux (hoy solo se ha definido el proyecto de CMake).
- Ajustar el sonido de cada modelo con escucha y mediciones.

## Estructura del repositorio

```
Source/          código del plugin (C++ / JUCE): procesador y editor
CMakeLists.txt   proyecto de CMake
assets/ui/       capas gráficas en PNG, manifest.json y vistas previas
docs/            especificación de la interfaz, guía de válvulas y notas de DSP
AGENTS.md        resumen técnico del proyecto para asistentes de programación
```

## Compilar

Requisitos: CMake 3.22 o superior, un compilador con C++17 y [JUCE](https://juce.com).

1. Descarga JUCE y colócalo en una carpeta `JUCE/` en la raíz del repositorio (la carpeta está en `.gitignore`).
2. Configura y compila:

```
cmake -B build
cmake --build build --config Release
```

El resultado es el plugin VST3 de KOMPRUSSR KU-01. La copia automática a la carpeta de plugins está desactivada.

## Licencia

El código se publica bajo la licencia **GNU AGPL v3** (ver `LICENSE`), porque usa el framework [JUCE](https://juce.com) bajo sus términos de código abierto (AGPLv3). Salvo que se indique otra cosa, las imágenes de `assets/ui` se publican con la misma licencia.

KOMPRUSSR es un proyecto independiente y no está afiliado a ningún fabricante de equipos ni de válvulas.
