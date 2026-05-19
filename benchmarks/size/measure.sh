#!/usr/bin/env bash
# Builds a minimal consumer (distance + point-in-polygon) against each library
# we benchmark, then reports the stripped binary size. Also reports the
# install footprint of the library itself (Homebrew prefix size, when
# available) so the reader can see the *deployment* cost, not just the link
# cost.
#
# Override:
#   CXX=clang++ ./measure.sh
#   CXXFLAGS="-std=c++17 -O3 -DNDEBUG" ./measure.sh
#   STATIC=1 ./measure.sh    # add -static (best-effort; not all toolchains)

set -euo pipefail

CXX="${CXX:-c++}"
CXXFLAGS="${CXXFLAGS:--std=c++17 -O2 -DNDEBUG}"
EXTRA_LD=""
if [ "${STATIC:-0}" = "1" ]; then
    EXTRA_LD="-static"
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
TMP="$(mktemp -d)"
LOG_DIR="${ROOT_DIR}/build-bench/size-logs"
# build() runs inside command substitution, so plain shell variables can't
# propagate failure state back to the parent. Use a sentinel file instead.
FAILURE_MARKER="${TMP}/.any_failure"
trap 'rm -rf "${TMP}"' EXIT

is_macos() { [ "$(uname)" = "Darwin" ]; }

file_size() {
    if is_macos; then stat -f '%z' "$1"; else stat -c '%s' "$1"; fi
}

human() {
    awk -v b="$1" 'BEGIN {
        split("B KB MB GB", u);
        i = 1;
        while (b >= 1024 && i < 4) { b /= 1024; i++ }
        printf "%.1f %s", b, u[i]
    }'
}

dir_size() {
    if [ -z "$1" ] || [ ! -e "$1" ]; then echo 0; return; fi
    # -L follows symlinks (Homebrew's --prefix is a symlink into Cellar/<ver>).
    if is_macos; then
        du -skL "$1" 2>/dev/null | awk '{print $1 * 1024}'
    else
        du -sbL "$1" 2>/dev/null | awk '{print $1}'
    fi
}

row() {
    # name, binary_size_bytes ("" if skipped), install_size_bytes ("" if N/A), notes
    local name="$1" bin="$2" inst="$3" notes="$4"
    local bin_str="—" inst_str="—"
    [ -n "${bin}"  ] && bin_str=$(human "${bin}")
    [ -n "${inst}" ] && inst_str=$(human "${inst}")
    printf "  %-22s %14s %14s   %s\n" "${name}" "${bin_str}" "${inst_str}" "${notes}"
}

build() {
    # name, source, extra_flags...
    local name="$1"; shift
    local source="$1"; shift
    local safe_name="${name// /_}"
    local out="${TMP}/${safe_name}"
    local log="${TMP}/${safe_name}.log"
    if "${CXX}" ${CXXFLAGS} ${EXTRA_LD} "${source}" -o "${out}" "$@" 2>"${log}"; then
        strip "${out}" 2>/dev/null || true
        file_size "${out}"
    else
        # Persist the failure log so the user can diagnose after the trap
        # cleans up TMP.
        mkdir -p "${LOG_DIR}"
        cp "${log}" "${LOG_DIR}/${safe_name}.log"
        : > "${FAILURE_MARKER}"
        echo ""  # signals "skipped"
    fi
}

brew_prefix() {
    command -v brew >/dev/null 2>&1 && brew --prefix "$1" 2>/dev/null || true
}

echo "Stripped binary + install-footprint comparison"
echo "  CXX=${CXX}, CXXFLAGS=${CXXFLAGS}${EXTRA_LD:+ ${EXTRA_LD}}"
echo
printf "  %-22s %14s %14s   %s\n" "Library" "Binary" "Installed" "Notes"
printf "  %-22s %14s %14s   %s\n" "-------" "------" "---------" "-----"

# --- geo-utils-cpp (header-only, in-tree) ----------------------------------
GU_BIN=$(build "geo-utils-cpp" "${SCRIPT_DIR}/consumer_geo_utils.cpp" \
    -I "${ROOT_DIR}/include")
GU_INST=$(dir_size "${ROOT_DIR}/include")
row "geo-utils-cpp" "${GU_BIN}" "${GU_INST}" "header-only, zero deps"

# --- naive (no library) ----------------------------------------------------
NV_BIN=$(build "naive" "${SCRIPT_DIR}/consumer_naive.cpp")
row "naive (no library)" "${NV_BIN}" "0" "hand-written haversine + planar PIP"

# --- S2 Geometry -----------------------------------------------------------
S2_FLAGS=""
S2_PFX="$(brew_prefix s2geometry)"
if [ -n "${S2_PFX}" ] && [ -d "${S2_PFX}" ]; then
    S2_FLAGS="-I${S2_PFX}/include -L${S2_PFX}/lib -ls2"
    # absl is a runtime peer; common labels:
    ABSL_PFX="$(brew_prefix abseil)"
    [ -n "${ABSL_PFX}" ] && S2_FLAGS="${S2_FLAGS} -I${ABSL_PFX}/include -L${ABSL_PFX}/lib"
elif command -v pkg-config >/dev/null && pkg-config --exists s2 2>/dev/null; then
    S2_FLAGS="$(pkg-config --cflags --libs s2)"
fi
if [ -n "${S2_FLAGS}" ]; then
    S2_BIN=$(build "S2 Geometry" "${SCRIPT_DIR}/consumer_s2.cpp" ${S2_FLAGS})
    S2_INST=$(dir_size "${S2_PFX}")
    ABSL_INST=$(dir_size "${ABSL_PFX:-}")
    S2_TOTAL=$((S2_INST + ABSL_INST))
    row "S2 Geometry" "${S2_BIN}" "${S2_TOTAL}" "+ abseil dependency"
else
    row "S2 Geometry" "" "" "not installed (brew install s2geometry)"
fi

# --- Boost.Geometry --------------------------------------------------------
BOOST_FLAGS=""
BOOST_PFX="$(brew_prefix boost)"
if [ -n "${BOOST_PFX}" ] && [ -d "${BOOST_PFX}/include/boost" ]; then
    BOOST_FLAGS="-I${BOOST_PFX}/include"
elif [ -d /usr/include/boost ]; then
    BOOST_FLAGS=""
    BOOST_PFX="/usr"
fi
if [ -n "${BOOST_PFX}" ] && [ -d "${BOOST_PFX}" ]; then
    B_BIN=$(build "Boost.Geometry" "${SCRIPT_DIR}/consumer_boost.cpp" ${BOOST_FLAGS})
    # Whole Boost is huge; report only Boost.Geometry headers as a fairer figure.
    BG_HDRS="${BOOST_PFX}/include/boost/geometry"
    B_INST=$(dir_size "${BG_HDRS}")
    row "Boost.Geometry" "${B_BIN}" "${B_INST}" "headers-only subset of Boost"
else
    row "Boost.Geometry" "" "" "not installed (brew install boost)"
fi

# --- GeographicLib ---------------------------------------------------------
GL_FLAGS=""
GL_PFX="$(brew_prefix geographiclib)"
if [ -n "${GL_PFX}" ] && [ -d "${GL_PFX}" ]; then
    GL_FLAGS="-I${GL_PFX}/include -L${GL_PFX}/lib -lGeographicLib"
elif command -v pkg-config >/dev/null && pkg-config --exists geographiclib 2>/dev/null; then
    GL_FLAGS="$(pkg-config --cflags --libs geographiclib)"
fi
if [ -n "${GL_FLAGS}" ]; then
    GL_BIN=$(build "GeographicLib" "${SCRIPT_DIR}/consumer_geographiclib.cpp" ${GL_FLAGS})
    GL_INST=$(dir_size "${GL_PFX}")
    row "GeographicLib" "${GL_BIN}" "${GL_INST}" "no native PIP — distance only"
else
    row "GeographicLib" "" "" "not installed (brew install geographiclib)"
fi

echo
echo "Notes:"
echo "  - 'Binary' is the stripped consumer executable (dynamic linking)."
echo "  - 'Installed' for geo-utils-cpp is the include/ directory (everything"
echo "    you ship). For other libraries it's the package install prefix on"
echo "    Homebrew (or the geometry subset for Boost) — what the user must"
echo "    have on disk to consume the library."
if [ -f "${FAILURE_MARKER}" ]; then
    echo "  - Some builds failed. Diagnostic logs preserved in: ${LOG_DIR}"
fi
