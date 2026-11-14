# HasiHasi – Skript zum Kompilieren von .ts-Files zu .qm-Files
# SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

#!/bin/bash

# Verzeichnis für Translation-Files
TRANSLATION_DIR="translations"

# Qt Linguist Tools
lupdate() {
  echo "Running lupdate..."
  lupdate -no-obsolete . -ts ${TRANSLATION_DIR}/*.ts
}

lrelease() {
  echo "Running lrelease..."
  lrelease ${TRANSLATION_DIR}/*.ts
}

# .ts-Files zu .qm-Files kompilieren
compile_translations() {
  echo "Compiling translations..."
  
  # Prüfen, ob lrelease verfügbar ist
  if ! command -v lrelease &> /dev/null; then
    echo "Error: lrelease not found. Please install Qt6."
    exit 1
  fi
  
  # .ts-Files zu .qm-Files kompilieren
  lrelease ${TRANSLATION_DIR}/*.ts
  
  if [ $? -eq 0 ]; then
    echo "Translations compiled successfully."
  else
    echo "Error: Failed to compile translations."
    exit 1
  fi
}

# Hauptprogramm
case "$1" in
  update)
    lupdate
    ;;
  release)
    lrelease
    ;;
  compile)
    compile_translations
    ;;
  *)
    echo "Usage: $0 {update|release|compile}"
    echo ""
    echo "Commands:"
    echo "  update    - Update .ts-Files from source code"
    echo "  release   - Compile .ts-Files to .qm-Files"
    echo "  compile   - Compile .ts-Files to .qm-Files (same as release)"
    exit 1
    ;;
esac
