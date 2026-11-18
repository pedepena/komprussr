#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
plugin=$(find "$script_dir" -maxdepth 1 -type d -name '*.vst3' -print -quit)
if [ -z "$plugin" ]; then
    echo "No se encontró el bundle VST3 junto a este script. / No VST3 bundle was found next to this script." >&2
    exit 1
fi

destination="${HOME:?HOME is not set}/.vst3"
name=${plugin##*/}
target="$destination/$name"
temporary="$destination/.${name}.installing.$$"

mkdir -p "$destination"
rm -rf "$temporary"
cp -R "$plugin" "$temporary"
rm -rf "$target"
mv "$temporary" "$target"

echo "KOMPRUSSR KU-01 instalado en: $target"
echo "KOMPRUSSR KU-01 installed to: $target"
echo "Vuelve a escanear los plugins en tu programa de música. / Rescan plug-ins in your music software."
