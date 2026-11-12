# Modelos de válvula: primera aproximación DSP

El selector conserva las cinco posiciones definidas por la interfaz: 6386U, 6BA6, 6BC8, 6K7 y 6SK7. La opción 6BZ8 queda fuera hasta disponer de hoja de datos y circuito de referencia verificados.

## Integración

`tube` es un `AudioParameterChoice` del `AudioProcessorValueTreeState`: el host puede automatizarlo y `getStateInformation`/`setStateInformation` lo guardan con el estado. La elección solo cambia rasgos del modelo; no reescribe Compression, Make Up Gain ni Power. Los perfiles están en `KomprussrAudioProcessor::modelFor`.

## Qué representa el DSP

El umbral recorre de -6 a -40 dBFS entre Compression 0 % y 100 %, más el desplazamiento propio de cada perfil. La compresión está inactiva a 0 %; entre 0 y 100 %, la ratio interpola desde 1,25:1 hasta el máximo del perfil: 2:1 para 6386U, 2,5:1 para 6BA6, 3:1 para 6BC8, 2,8:1 para 6K7 y 4:1 para 6SK7. Cada perfil define además knee, ataque, recuperación rápida, recuperación lenta y el punto de transición entre ambas recuperaciones.

La señal comprimida alimenta una saturación sobremuestreada. Su intensidad depende de la reducción de ganancia aplicada y del nivel detectado; con Compression 0 % no añade saturación. Se conserva un filtro de bloqueo de continua, pero se ha retirado el paso bajo fijo por válvula para que el carácter no dependa de una EQ estática. Los bloques mayores que el bloque preparado se procesan por fragmentos, sin reservar memoria en `processBlock`.

Los perfiles son decisiones creativas iniciales inspiradas en las válvulas, no emulaciones físicas ni ajustes obtenidos de circuitos concretos. Deben afinarse mediante escucha y mediciones.

## Comprobación reproducible

Con un host de audio, crea cinco instancias o renders con idénticos ajustes, señal, nivel de entrada y duración; cambia únicamente Tube. Usa un seno de 1 kHz a -18 dBFS y después una ráfaga/ruido con picos entre -12 y -3 dBFS, Compression al 70 %, Make Up Gain a 0 dB y Power activado. Registra:

1. RMS/peak de salida y reducción de ganancia estacionaria para medir las curvas y compensación.
2. Tiempo de subida y recuperación de la reducción al medir la envolvente de salida de la ráfaga.
3. FFT de la salida para comparar armónicos 2.º–5.º con el tono de entrada y con Power apagado.
4. Nivel integrado (LUFS o RMS) al igualar salidas para escucha ciega; anota si los modelos siguen siendo distinguibles.

Una diferencia audible o espectral es esperable por construcción, pero no se considera validación de calidad. El DSP requiere escucha crítica y medidas antes de usarse en mezclas finales. Este directorio aún no incluye JUCE; configurar/compilar requiere instalar JUCE y apuntar `JUCE_DIR` al paquete CMake.

## Referencias técnicas

- Tung-Sol, hoja de datos 6386: https://frank.pocnet.net/sheets/142/6/6386.pdf
- RCA, Receiving Tube Manual RC-19 (datos 6BA6/6BC8): https://frank.pocnet.net/other/RCA/RC-Series/RCA_RC19.pdf
- RCA, Receiving Tube Manual (datos 6K7/6SK7): https://www.qsl.net/n9zia/RCA_Receiving_Tube_Manual.pdf

Las hojas apoyan la identificación y los datos eléctricos de las válvulas; no justifican directamente los coeficientes DSP elegidos aquí.
