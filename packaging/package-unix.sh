#!/usr/bin/env bash
# Uso: bash packaging/package-unix.sh <macOS|Linux> <version>
# Se ejecuta con "bash", asi que no necesita permiso de ejecucion en git.
set -euo pipefail

SISTEMA="${1:?Falta el sistema (macOS o Linux)}"
VERSION="${2:-1.0.0}"
NOMBRE="KOMPRUSSR KU-01"
SALIDA="build/package"

rm -rf "$SALIDA"
mkdir -p "$SALIDA"

# Busca un bundle por nombre dentro de build/ (no depende de la carpeta exacta).
buscar() { find build -type d -name "$1" | sed -n '1p'; }

VST3="$(buscar "$NOMBRE.vst3")"
if [ -z "$VST3" ]; then echo "No se encontro $NOMBRE.vst3 en build/" >&2; exit 1; fi

if [ "$SISTEMA" = "macOS" ]; then
  AU="$(buscar "$NOMBRE.component")"
  if [ -z "$AU" ]; then echo "No se encontro $NOMBRE.component en build/ (falta AU en CMake?)" >&2; exit 1; fi

  RAIZ="$SALIDA/raiz"
  mkdir -p "$RAIZ/Library/Audio/Plug-Ins/VST3" "$RAIZ/Library/Audio/Plug-Ins/Components"
  ditto "$VST3" "$RAIZ/Library/Audio/Plug-Ins/VST3/$NOMBRE.vst3"
  ditto "$AU" "$RAIZ/Library/Audio/Plug-Ins/Components/$NOMBRE.component"
  test -d "$RAIZ/Library/Audio/Plug-Ins/VST3/$NOMBRE.vst3"
  test -d "$RAIZ/Library/Audio/Plug-Ins/Components/$NOMBRE.component"

  # Firma "ad-hoc" (sin cuenta de Apple): evita que el plugin quede marcado como danado.
  codesign --force --deep --sign - "$RAIZ/Library/Audio/Plug-Ins/VST3/$NOMBRE.vst3"
  codesign --force --deep --sign - "$RAIZ/Library/Audio/Plug-Ins/Components/$NOMBRE.component"

  # Que el instalador siempre ponga los plugins en /Library/Audio/Plug-Ins y los actualice.
  pkgbuild --analyze --root "$RAIZ" "$SALIDA/componentes.plist"
  python3 - "$SALIDA/componentes.plist" <<'PY'
import plistlib, sys
ruta = sys.argv[1]
with open(ruta, "rb") as f:
    datos = plistlib.load(f)
for item in datos:
    item["BundleIsRelocatable"] = False
    item["BundleOverwriteAction"] = "upgrade"
with open(ruta, "wb") as f:
    plistlib.dump(datos, f)
PY

  pkgbuild --root "$RAIZ" \
    --component-plist "$SALIDA/componentes.plist" \
    --identifier com.pedepena.komprussr.instalador \
    --version "$VERSION" \
    --install-location / \
    "$SALIDA/KOMPRUSSR-KU-01-macOS-$VERSION.pkg"

  rm -rf "$RAIZ" "$SALIDA/componentes.plist"
else
  PAQUETE="KOMPRUSSR-KU-01-Linux-$VERSION"
  DEST="$SALIDA/$PAQUETE"
  mkdir -p "$DEST"
  cp -R "$VST3" "$DEST/"

  cat > "$DEST/instalar.sh" <<'EOF'
#!/usr/bin/env bash
# Instala KOMPRUSSR KU-01 solo para tu usuario (~/.vst3).
# Para todos los usuarios: ./instalar.sh sistema
set -e
cd "$(dirname "$0")"
if [ "${1:-}" = "sistema" ]; then DESTINO="/usr/lib/vst3"; SUDO="sudo"; else DESTINO="$HOME/.vst3"; SUDO=""; fi
$SUDO mkdir -p "$DESTINO"
$SUDO rm -rf "$DESTINO/KOMPRUSSR KU-01.vst3"
$SUDO cp -R "KOMPRUSSR KU-01.vst3" "$DESTINO/"
echo "Instalado en $DESTINO. Reinicia tu DAW y vuelve a buscar plugins."
EOF
  chmod +x "$DEST/instalar.sh"

  cat > "$DEST/LEEME.txt" <<'EOF'
KOMPRUSSR KU-01 (VST3 para Linux)

1. Abre una terminal en esta carpeta.
2. Ejecuta: ./instalar.sh
   (o ./instalar.sh sistema para instalarlo para todos los usuarios)
3. Reinicia tu DAW y vuelve a buscar plugins.

Para desinstalar, borra la carpeta "KOMPRUSSR KU-01.vst3" de ~/.vst3 (o de /usr/lib/vst3).
EOF

  tar -czf "$SALIDA/$PAQUETE.tar.gz" -C "$SALIDA" "$PAQUETE"
  rm -rf "$DEST"
fi

echo "Paquetes creados en $SALIDA:"
ls -1 "$SALIDA"
