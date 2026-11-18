#!/usr/bin/env bash
set -euo pipefail

system="$1"
version="$2"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="$repo_root/build/KOMPRUSSR_artefacts/Release"
package_root="$repo_root/dist/staging"
output="$repo_root/dist"
mkdir -p "$package_root" "$output"

find_bundle() {
  local format="$1"
  local suffix="$2"
  find "$build_root" -type d -path "*/$format/*" -name "*$suffix" -print -quit
}

if [[ "$system" == "macOS" ]]; then
  vst3="$(find_bundle VST3 '.vst3')"
  au="$(find_bundle AU '.component')"
  [[ -n "$vst3" && -n "$au" ]] || { echo "Could not find both VST3 and AU bundles under $build_root" >&2; exit 1; }

  stage="$package_root/KOMPRUSSR-KU-01-${version}-macOS"
  rm -rf "$stage"
  mkdir -p "$stage/VST3" "$stage/AU"
  cp -R "$vst3" "$stage/VST3/"
  cp -R "$au" "$stage/AU/"
  cp "$repo_root/LICENSE" "$stage/"
  cat > "$stage/LEEME.txt" <<'EOF'
KOMPRUSSR KU-01 — macOS (Apple Silicon e Intel / Apple Silicon and Intel)

PAQUETE PORTÁTIL / PORTABLE PACKAGE
Incluye VST3 y Audio Unit (AU). Este ZIP no instala automáticamente los plugins.
Includes VST3 and Audio Unit (AU). This ZIP does not install the plug-ins automatically.

VST3: copia «VST3/Komprussr KU-01.vst3» a ~/Library/Audio/Plug-Ins/VST3/.
AU: copia «AU/Komprussr KU-01.component» a ~/Library/Audio/Plug-Ins/Components/.
Crea las carpetas si no existen y vuelve a escanear los plugins en tu programa de música. Logic usa AU; otras aplicaciones pueden usar VST3.

VST3: copy “VST3/Komprussr KU-01.vst3” to ~/Library/Audio/Plug-Ins/VST3/.
AU: copy “AU/Komprussr KU-01.component” to ~/Library/Audio/Plug-Ins/Components/.
Create the folders if needed and rescan plug-ins in your music software. Logic uses AU; other applications may use VST3.

INSTALADOR / INSTALLER
El archivo .pkg instala ambos formatos en las carpetas comunes del sistema. Haz doble clic y sigue los pasos.
The .pkg installs both formats in the system plug-in folders. Double-click it and follow the prompts.

Los paquetes no están firmados ni notarizados. Si macOS los bloquea, haz clic con Control y elige «Abrir», o usa «Ajustes del Sistema > Privacidad y seguridad > Abrir igualmente» cuando aparezca. / Packages are not signed or notarized. If macOS blocks them, Control-click and choose “Open”, or use “System Settings > Privacy & Security > Open Anyway” when offered.
Ayuda / Help: https://github.com/pedepena/komprussr
EOF
  ditto -c -k --sequesterRsrc --keepParent "$stage" "$output/KOMPRUSSR-KU-01-${version}-macOS.zip"

  pkgroot="$package_root/pkgroot"
  rm -rf "$pkgroot"
  mkdir -p "$pkgroot/Library/Audio/Plug-Ins/VST3" "$pkgroot/Library/Audio/Plug-Ins/Components" \
    "$pkgroot/usr/local/share/licenses/KOMPRUSSR-KU-01"
  cp -R "$vst3" "$pkgroot/Library/Audio/Plug-Ins/VST3/"
  cp -R "$au" "$pkgroot/Library/Audio/Plug-Ins/Components/"
  cp "$repo_root/LICENSE" "$pkgroot/usr/local/share/licenses/KOMPRUSSR-KU-01/"
  pkgbuild --root "$pkgroot" --identifier com.pedepena.komprussr --version "$version" \
    --install-location / --ownership recommended "$output/KOMPRUSSR-KU-01-${version}-macOS.pkg"
elif [[ "$system" == "Linux" ]]; then
  vst3="$(find_bundle VST3 '.vst3')"
  [[ -n "$vst3" ]] || { echo "Could not find a VST3 bundle under $build_root" >&2; exit 1; }

  stage="$package_root/KOMPRUSSR-KU-01-${version}-Linux-x86_64"
  rm -rf "$stage"
  mkdir -p "$stage"
  cp -R "$vst3" "$stage/"
  cp "$repo_root/installer/linux/install.sh" "$repo_root/installer/linux/LEEME.txt" "$repo_root/LICENSE" "$stage/"
  chmod +x "$stage/install.sh"
  tar -czf "$output/KOMPRUSSR-KU-01-${version}-Linux-x86_64.tar.gz" -C "$package_root" "$(basename "$stage")"
else
  echo "Unsupported packaging system: $system" >&2
  exit 2
fi

if command -v shasum >/dev/null 2>&1 || command -v sha256sum >/dev/null 2>&1; then
  package_files=()
  for package in "$output"/KOMPRUSSR-KU-01-"$version"-*; do
    [[ -f "$package" && "$package" != *.sha256 ]] && package_files+=("$(basename "$package")")
  done
  if ((${#package_files[@]})); then
    if command -v shasum >/dev/null 2>&1; then
      (cd "$output" && shasum -a 256 "${package_files[@]}") > "$output/KOMPRUSSR-KU-01-${version}-${system}.sha256"
    else
      (cd "$output" && sha256sum "${package_files[@]}") > "$output/KOMPRUSSR-KU-01-${version}-${system}.sha256"
    fi
  fi
fi
