# Historial de cambios

## 1.0.1 — compatibilidad de instaladores
- Linux se compila en Ubuntu 22.04, enlaza estáticamente libstdc++ y libgcc cuando es posible, y el workflow comprueba que el plugin no requiere GLIBC posterior a 2.36.
- Windows usa el runtime de Visual C++ estático; el workflow comprueba sus dependencias. El instalador avisa y se detiene si no es Windows 10/11 de 64 bits.
- macOS mantiene el binario universal para Intel y Apple Silicon, con objetivo mínimo macOS 10.15. El workflow comprueba arquitecturas, dependencias y la presencia de VST3 y AU en el `.pkg` y el `.zip`.
- Se conservan los cinco formatos de descarga; esta versión no reemplaza la 1.0.0.
- Requisitos y avisos de SmartScreen y Gatekeeper documentados en el README y en las notas de release.

**No probado todavía:** macOS y Linux en hosts reales; la publicación requiere ejecutar correctamente el workflow de release.

## 1.0.0 — primera versión pública
<!-- La version 1.0.0 corresponde al tag historico v1.0.0. -->

- Compresor valvular tipo Vari-Mu con los controles Compression, Make Up Gain, Tube y Power.
- Cinco modelos de válvula inspirados en: 6386U, 6BA6, 6BC8, 6K7 y 6SK7.
- Medidor de reducción de ganancia (GR) con aguja.
- Interfaz gráfica escalable de estética soviética de los años 50 y 60, que recuerda su último tamaño.
- Parámetros automatizables que se guardan con el proyecto.
- Engranaje con las pestañas About y Manual, y comprobación opcional de nuevas versiones.
- Formatos: VST3 (Windows, macOS y Linux) y AU (macOS).
- Manual de usuario en español.

**Probado en:** FL Studio (Windows). Aún no se ha probado en macOS, Linux ni en otros programas de música.

**Notas:**
- Mientras el instalador de Windows no esté firmado, Windows mostrará el aviso «Windows protegió su equipo» (SmartScreen); el README explica cómo continuar. También se ofrece un `.zip` sin instalador.
- Con Power OFF puede haber un desfase mínimo al alternar ON/OFF (ver README, «Límites conocidos»).
