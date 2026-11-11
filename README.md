# KOMPRUSSR KU-01

Compresor valvular tipo Vari-Mu en formato de plugin de audio (VST3), con selector de modelos de válvula y una interfaz de estética soviética de los años 50 y 60: chapa roja desgastada, knobs chicken head de baquelita y medidor de reducción de ganancia (GR).

> **Estado:** en desarrollo. El diseño gráfico está terminado y el motor de audio tiene una primera versión funcional. La interfaz gráfica final (con las capas PNG) aún no está integrada en el código.

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

- **Ahora:** una interfaz provisional con controles estándar de JUCE (selector, dos knobs, interruptor y texto de GR).
- **Prevista:** lienzo de 1376 × 768 px escalable, hecho con capas PNG (`assets/ui`), knobs que giran, selector de válvula, tumbler ON/OFF, válvulas que se iluminan y aguja de GR calibrada.

## Pendiente

- Integrar la interfaz gráfica final con las capas PNG.
- Decidir el valor por defecto de Compression, el calentamiento de las válvulas al encender y el comportamiento de los controles en OFF.
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
