# KOMPRUSSR KU-01

Compresor valvular tipo Vari-Mu en formato de plugin de audio (VST3 y AU), con selector de modelos de válvula y una interfaz de estética soviética de los años 50 y 60: chapa roja desgastada, knobs chicken head de baquelita y medidor de reducción de ganancia (GR).

> **Estado:** en desarrollo. El diseño gráfico está terminado; el motor de audio y la interfaz en código están pendientes.

## Características previstas

- **Compression:** 0 a 100 %.
- **Make Up Gain:** de -12 a +24 dB (0 dB por defecto).
- **Tube:** 5 modelos de válvula — 6386U, 6BA6, 6BC8, 6K7 y 6SK7. Cada uno cambia la curva de compresión, la respuesta dinámica y el color armónico (modelos *inspirados en* esas válvulas; no son emulaciones exactas de ningún equipo concreto).
- **ON/OFF:** al apagar, las válvulas se apagan; al encender, se vuelven a iluminar.
- **Medidor GR:** aguja de reducción de ganancia.
- Formatos: VST3 y AU. Windows, macOS y Linux.

## Estructura del repositorio

```
Source/          código del plugin (C++ / JUCE)
assets/ui/       capas gráficas en PNG, manifest.json y vistas previas
docs/            especificación de la interfaz y guía de válvulas
AGENTS.md        resumen técnico del proyecto para asistentes de programación
```

## Compilar

Pendiente. Se documentará aquí cuando exista el proyecto de JUCE.

## Licencia

El código se publica bajo la licencia **GNU AGPL v3** (ver `LICENSE`), porque usa el framework [JUCE](https://juce.com) bajo sus términos de código abierto (AGPLv3). Salvo que se indique otra cosa, las imágenes de `assets/ui` se publican con la misma licencia.

KOMPRUSSR es un proyecto independiente y no está afiliado a ningún fabricante de equipos ni de válvulas.
