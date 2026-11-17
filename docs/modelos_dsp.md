# Modelos de válvula: primera aproximación DSP

El selector conserva las cinco posiciones definidas por la interfaz: 6386U, 6BA6, 6BC8, 6K7 y 6SK7. La opción 6BZ8 queda fuera hasta disponer de hoja de datos y circuito de referencia verificados.

## Integración

`tube` es un `AudioParameterChoice` del `AudioProcessorValueTreeState`: el host puede automatizarlo y `getStateInformation`/`setStateInformation` lo guardan con el estado. La elección solo cambia rasgos del modelo; no reescribe Compression, Make Up Gain ni Power. Los perfiles están en `KomprussrAudioProcessor::modelFor`.

## Qué representa el DSP

El umbral usa `-6 - 34 * compression^0,60` dBFS más el desplazamiento del perfil. Esta curva adelanta el comienzo de la compresión y conserva los extremos de -6 y -40 dBFS. La compresión está inactiva a 0 %. La ratio interpola desde 1,25:1 hasta el máximo del perfil mediante `compression^curveExponent`.

La envolvente RMS usa una ventana rápida fija; dos seguidores en dB aplican el ataque del perfil y recuperaciones rápida/lenta. La mezcla deja una cola lenta, sin el segundo suavizado de ganancia anterior. La señal comprimida alimenta la saturación sobremuestreada. El color escala con la reducción real y lleva una base muy pequeña cuando Compression está activo; a Compression 0 % queda en cero. El balance par/impar varía con el nivel, cada válvula tiene una curva de saturación y grano transitorio propios, y la etapa de tono abre por completo sin reducción y se acerca al corte del perfil según la reducción aplicada.

Los coeficientes exponenciales se calculan una vez por bloque, no por muestra. La conversión de reducción de dB a ganancia lineal usa una tabla interpolada que se prepara fuera del hilo de audio; el pequeño ajuste de salida se calcula por bloque. Los bloques mayores que el bloque preparado se procesan por fragmentos con buffers reservados en `prepareToPlay`. No se añade filtro al detector.

La saturación usa un nivel de referencia propio por perfil antes de aplicar la curva: 2,5 para 6386U, 3,0 para 6BA6, 6BC8 y 6K7, y 2,0 para 6SK7. La salida de la curva se normaliza con el mismo pre-gain para conservar ganancia lineal unitaria. Además, el pre-gain aumenta con la reducción mediante `dbToGain(0,3 * measuredReduction)` y se compensa después. Así la compresión puede aumentar el carácter armónico aunque reduzca el nivel de señal.

Cada perfil puede saturar toda la banda o solo una zona. Un filtro de un polo separa graves y resto de banda a la frecuencia sobremuestreada; el tramo no seleccionado se suma sin saturación. El estado del filtro se conserva por válvula y canal y no añade latencia ni buffers dinámicos. Con color cero, la contribución de saturación es nula.

## Perfiles DSP

Los tiempos y coeficientes son puntos de partida creativos. `Reparto lento` es la proporción del seguidor lento en la mezcla de reducción; la proporción restante viene del seguidor rápido. `Hardness`, `grano` y `asimetría` son coeficientes internos sin unidades.

| Válvula | Umbral offset (dB) | Ratio máxima | Exponente ratio | Knee (dB) | Ataque (ms) | Recuperación rápida (ms) | Lenta (ms) | Reparto lento | Corte de tono (Hz) |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 6386U | 0,0 | 2,0 | 0,8 | 8,0 | 32 | 150 | 1500 | 0,60 | 9000 |
| 6BA6 | -1,0 | 2,5 | 1,0 | 6,0 | 20 | 100 | 900 | 0,50 | 8200 |
| 6BC8 | -1,5 | 3,0 | 1,2 | 4,5 | 12 | 70 | 600 | 0,45 | 7400 |
| 6K7 | 0,5 | 2,8 | 0,9 | 5,5 | 24 | 110 | 1000 | 0,50 | 6700 |
| 6SK7 | -0,5 | 4,0 | 1,5 | 3,5 | 9 | 50 | 450 | 0,35 | 6000 |

| Válvula | Drive armónico | Drive máximo | Par | Impar | Hardness | Grano transitorio | Asimetría | Referencia | Modo/frecuencia de saturación | Trim de salida (dB) |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|
| 6386U | 0,35 | 0,40 | 0,72 | 0,28 | 0,00 | 0,00 | 0,000 | 2,5 | banda completa | -0,78 |
| 6BA6 | 0,50 | 0,55 | 0,55 | 0,45 | 0,15 | 0,15 | 0,120 | 3,0 | graves hasta 800 Hz | -0,15 |
| 6BC8 | 0,48 | 0,60 | 0,62 | 0,38 | 0,30 | 0,35 | 0,200 | 3,0 | graves hasta 400 Hz | 1,19 |
| 6K7 | 0,55 | 0,60 | 0,48 | 0,60 | 0,35 | 0,10 | -0,050 | 3,0 | agudos desde 300 Hz | -0,38 |
| 6SK7 | 1,45 | 0,85 | 0,42 | 0,58 | 0,60 | 0,30 | 0,040 | 2,0 | agudos desde 800 Hz | 0,00 |

Los trims se aplican progresivamente con la reducción de ganancia, por lo que son cero sin reducción y Compression 0 % permanece transparente en nivel. Los trims indicados son puntos de partida; deben reajustarse con renders del plugin para acercar las salidas a ±1 dB con Compression 70 %.

Los perfiles, el nivel de referencia, los filtros de banda y los trims son decisiones creativas inspiradas en las válvulas, no mediciones de válvulas ni ajustes de circuitos concretos. Deben afinarse mediante escucha y mediciones.

## Comprobación reproducible

Con un host de audio, crea cinco instancias o renders con idénticos ajustes, señal, nivel de entrada y duración; cambia únicamente Tube. Usa un seno de 1 kHz a -18 dBFS y después una ráfaga/ruido con picos entre -12 y -3 dBFS, Compression al 70 %, Make Up Gain a 0 dB y Power activado. Registra:

1. RMS/peak de salida y reducción de ganancia estacionaria para medir las curvas y compensación.
2. Tiempo de subida y recuperación de la reducción al medir la envolvente de salida de la ráfaga.
3. FFT de la salida para comparar armónicos 2.º–5.º con el tono de entrada y con Power apagado.
4. Nivel integrado (LUFS o RMS) al igualar salidas para escucha ciega; anota si los modelos siguen siendo distinguibles.

Las cifras de reducción y armónicos deben medirse de nuevo tras estos cambios: las tablas de la iteración anterior ya no representan los perfiles actuales. La estabilidad en ejecución y la escucha en host también requieren comprobación.

Una diferencia audible o espectral es esperable por construcción, pero no se considera validación de calidad. El DSP requiere escucha crítica y medidas antes de usarse en mezclas finales. Configurar/compilar requiere JUCE disponible en `JUCE_DIR` o como subdirectorio del proyecto.

## Referencias técnicas

- Tung-Sol, hoja de datos 6386: https://frank.pocnet.net/sheets/142/6/6386.pdf
- RCA, Receiving Tube Manual RC-19 (datos 6BA6/6BC8): https://frank.pocnet.net/other/RCA/RC-Series/RCA_RC19.pdf
- RCA, Receiving Tube Manual (datos 6K7/6SK7): https://www.qsl.net/n9zia/RCA_Receiving_Tube_Manual.pdf

Las hojas apoyan la identificación y los datos eléctricos de las válvulas; no justifican directamente los coeficientes DSP elegidos aquí.
