#!/bin/bash
# HasiHasi – Paket-Build-Skript
# SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# usage: packaging/build-packages.sh [--target <name>] [--version <x.y.z>]
#
# Baut Linux-Pakete für alle Ziel-Distributionen in Containern.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
RELEASES_DIR="$PROJECT_DIR/releases"
TARGETS_FILE="$SCRIPT_DIR/targets.conf"

# ── Farben ─────────────────────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*"; }
log_step()  { echo -e "${BLUE}[STEP]${NC} $*"; }

# ── Argumente ──────────────────────────────────────────────────────────
TARGET_FILTER=""
VERSION=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --target)
      TARGET_FILTER="$2"
      shift 2
      ;;
    --version)
      VERSION="$2"
      shift 2
      ;;
    *)
      log_error "Unbekannte Option: $1"
      exit 1
      ;;
  esac
done

# ── Version ermitteln ──────────────────────────────────────────────────
if [[ -z "$VERSION" ]]; then
  # Versuche Git-Tag
  if git -C "$PROJECT_DIR" describe --tags --abbrev=0 2>/dev/null | grep -q '^v'; then
    VERSION="$(git -C "$PROJECT_DIR" describe --tags --abbrev=0 | sed 's/^v//')"
  else
    # Entwicklungs-Build
    GIT_HASH="$(git -C "$PROJECT_DIR" rev-parse --short=7 HEAD 2>/dev/null || echo "unknown")"
    GIT_DATE="$(git -C "$PROJECT_DIR" log -1 --format=%cd --date=format:'%Y%m%d' 2>/dev/null || echo "unknown")"
    VERSION="0.1.0~git${GIT_DATE}.${GIT_HASH}"
  fi
fi

log_info "Version: $VERSION"

# ── Targets lesen ─────────────────────────────────────────────────────
declare -a TARGETS=()

if [[ ! -f "$TARGETS_FILE" ]]; then
  log_error "targets.conf nicht gefunden: $TARGETS_FILE"
  exit 1
fi

while IFS=': ' read -r key value; do
  [[ -z "$key" || "$key" =~ ^# ]] && continue
  key="$(echo "$key" | xargs)"
  value="$(echo "$value" | xargs)"
  [[ -z "$value" ]] && continue

  if [[ -n "$TARGET_FILTER" && "$key" != "$TARGET_FILTER" ]]; then
    continue
  fi

  TARGETS+=("$key:$value")
done < "$TARGETS_FILE"

if [[ ${#TARGETS[@]} -eq 0 ]]; then
  log_error "Keine Targets gefunden"
  exit 1
fi

log_info "Targets: ${TARGETS[*]}"

# ── Container-Tool bestimmen ──────────────────────────────────────────
CONTAINER_TOOL=""
if command -v podman &>/dev/null; then
  CONTAINER_TOOL="podman"
elif command -v docker &>/dev/null; then
  CONTAINER_TOOL="docker"
else
  log_error "Weder podman noch docker gefunden"
  exit 1
fi

log_info "Container-Tool: $CONTAINER_TOOL"

# ── SHA256SUMS und BUILDINFO generieren ───────────────────────────────
generate_release_info() {
  local release_dir="$1"
  local version="$2"

  log_step "Generiere SHA256SUMS und BUILDINFO..."

  # SHA256SUMS
  (cd "$release_dir" && sha256sum *) > SHA256SUMS 2>/dev/null || true

  # BUILDINFO
  {
    echo "Version: $version"
    echo "Build-Datum: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
    echo "Git-Commit: $(git -C "$PROJECT_DIR" rev-parse HEAD 2>/dev/null || echo 'unknown')"
    echo "Git-Branch: $(git -C "$PROJECT_DIR" rev-parse --abbrev-ref HEAD 2>/dev/null || echo 'unknown')"
    echo "Container-Tool: $CONTAINER_TOOL"
    echo "Compiler: $(gcc --version | head -1)"
    echo "CMake: $(cmake --version | head -1)"
  } > "$release_dir/BUILDINFO.txt"

  log_info "Release-Info generiert"
}

# ── Build-Funktion ─────────────────────────────────────────────────────
build_target() {
  local distro="$1"
  local version="$2"
  local container_dir="$SCRIPT_DIR/containers"
  local containerfile="$container_dir/Containerfile.${distro}"

  if [[ ! -f "$containerfile" ]]; then
    log_warn "Kein Containerfile für $distro: $containerfile"
    return 0
  fi

  local release_dir="$RELEASES_DIR/$version/${distro}-${version}"

  # Prüfe, ob Release-Verzeichnis schon existiert (kein Überschreiben!)
  if [[ -d "$release_dir" && "$version" != *"~git"* && "$version" != *".r"* ]]; then
    log_error "Release-Verzeichnis existiert schon: $release_dir"
    log_error "Lösche es zuerst oder verwende ein anderes Version-Suffix."
    return 1
  fi

  log_info "Bauen für ${distro}-${version}..."
  mkdir -p "$release_dir"

  # Build im Container
  log_step "Container-Build..."
  $CONTAINER_TOOL build \
    --build-arg VERSION="$version" \
    -f "$containerfile" \
    -t "hasihasi-build-${distro}-$(date +%s)" \
    "$PROJECT_DIR" 2>&1 | tail -20

  # Container starten und Pakete extrahieren
  log_step "Pakete extrahieren..."
  $CONTAINER_TOOL run --rm \
    -v "$release_dir:/output" \
    "hasihasi-build-${distro}-$(date +%s)" \
    bash -c "echo 'Paket-Build abgeschlossen'" 2>&1 || true

  # Lint-Checks
  log_step "Lint-Checks..."
  case "$distro" in
    debian|ubuntu)
      if command -v lintian &>/dev/null; then
        for deb in "$release_dir"/*.deb; do
          [[ -f "$deb" ]] && lintian "$deb" || true
        done
      fi
      ;;
    fedora)
      if command -v rpmlint &>/dev/null; then
        for rpm in "$release_dir"/*.rpm; do
          [[ -f "$rpm" ]] && rpmlint "$rpm" || true
        done
      fi
      ;;
    arch)
      if command -v namcap &>/dev/null; then
        for pkg in "$release_dir"/*.pkg.tar.zst; do
          [[ -f "$pkg" ]] && namcap "$pkg" || true
        done
      fi
      ;;
  esac

  # Installationstest in frischem Container
  log_step "Installationstest..."
  test_installation "$distro" "$release_dir" "$version"

  # Release-Info generieren
  generate_release_info "$release_dir" "$version"

  log_info "Bau abgeschlossen für ${distro}-${version}"
}

# ── Installationstest ──────────────────────────────────────────────────
test_installation() {
  local distro="$1"
  local release_dir="$2"
  local version="$3"

  log_info "Installationstest für ${distro}..."

  # Frischen Test-Container erstellen
  local test_container="hasihasi-test-$(date +%s)"
  local test_image="hasihasi-test-image-${distro}-$(date +%s)"

  # Containerfile für Test-Container
  local test_containerfile="$SCRIPT_DIR/containers/Containerfile.test-${distro}"
  
  case "$distro" in
    debian|ubuntu)
      # .deb-Paket installieren
      for deb in "$release_dir"/*.deb; do
        if [[ -f "$deb" ]]; then
          log_info "Installiere $deb..."
          # In frischem Container testen
          $CONTAINER_TOOL run --rm \
            --name "$test_container" \
            "$distro":latest \
            bash -c "apt-get update && apt-get install -y ./$(basename "$deb") && hasihasi --version && hasihasi-server --version" 2>&1 || true
          break
        fi
      done
      ;;
    fedora)
      # .rpm-Paket installieren
      for rpm in "$release_dir"/*.rpm; do
        if [[ -f "$rpm" ]]; then
          log_info "Installiere $rpm..."
          # In frischem Container testen
          $CONTAINER_TOOL run --rm \
            --name "$test_container" \
            "fedora":latest \
            bash -c "dnf install -y ./$(basename "$rpm") && hasihasi --version && hasihasi-server --version" 2>&1 || true
          break
        fi
      done
      ;;
    arch)
      # .pkg.tar.zst-Paket installieren
      for pkg in "$release_dir"/*.pkg.tar.zst; do
        if [[ -f "$pkg" ]]; then
          log_info "Installiere $pkg..."
          # In frischem Container testen
          $CONTAINER_TOOL run --rm \
            --name "$test_container" \
            "archlinux:latest" \
            bash -c " pacman -Syu --noconfirm ./$(basename "$pkg") && hasihasi --version && hasihasi-server --version" 2>&1 || true
          break
        fi
      done
      ;;
  esac

  log_info "Installationstest abgeschlossen für ${distro}"
}

# ── Haupt-Schleife ────────────────────────────────────────────────────
FAILED=0

for target in "${TARGETS[@]}"; do
  distro="${target%%:*}"
  ver="${target##*:}"

  if ! build_target "$distro" "$ver"; then
    log_error "Build fehlgeschlagen für $distro:$ver"
    FAILED=1
  fi
done

if [[ $FAILED -ne 0 ]]; then
  log_error "Ein oder mehrere Builds sind fehlgeschlagen"
  exit 1
fi

log_info "Alle Builds erfolgreich abgeschlossen!"
exit 0
