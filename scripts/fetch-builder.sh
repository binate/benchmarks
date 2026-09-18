#!/bin/sh
# Resolve a Binate toolchain bundle and print a path on stdout:
#
#   BNC="$(scripts/fetch-builder.sh)"             # bnc binary (default tool)
#   BNI="$(scripts/fetch-builder.sh --tool bni)"  # another tool's binary
#   LIB="$(scripts/fetch-builder.sh --lib)"       # stdlib + runtime root (bundle/lib)
#
# The bundle version comes from the BUILDER_VERSION file at the repo
# root, overridable with the BUILDER_VERSION env var. Two forms:
#
#   bnc-X.Y.Z   download that exact release from binate/binate.
#   latest      resolve the newest published bnc-* release, then use it.
#
# Bundles are downloaded once, sha256-verified against the release's
# SHA256SUMS, and cached under
#   ~/.cache/binate/builders/<version>/<os>-<arch>/bundle/
# (override the base with BINATE_CACHE_DIR) — the same layout the binate
# repo's fetcher uses, so the cache is shared. A cache hit skips the
# download (and is trusted without re-verifying).
#
# Set BINATE_BUNDLE=<dir> to bypass all of the above and use a pre-built
# bundle directly (e.g. one built from a binate `main` checkout) — no
# version, download, or verification. The directory must have the usual
# bundle layout (bin/<tool>, lib/). This is how to build the examples
# against a fresh `main` toolchain without cutting a release.
set -e

mode=bin
tool=bnc
while [ $# -gt 0 ]; do
    case "$1" in
        --lib)     mode=lib; shift ;;
        --tool)    tool="$2"; shift 2 ;;
        --tool=*)  tool="${1#--tool=}"; shift ;;
        -*)        echo "fetch-builder: unknown flag: $1" >&2; exit 2 ;;
        *)         echo "fetch-builder: unexpected arg: $1" >&2; exit 2 ;;
    esac
done
case "$tool" in
    bnc|bni|bnas|bnlint) ;;
    *) echo "fetch-builder: unknown --tool: $tool" >&2; exit 2 ;;
esac

# Bundle-path override: point at a pre-built bundle directly (e.g. one built
# from a binate `main` checkout) and skip the versioned download/verify/cache
# entirely. The directory must have the usual bundle layout (bin/<tool>, lib/).
# Takes precedence over BUILDER_VERSION; the default release path below is
# unchanged when BINATE_BUNDLE is unset.
if [ -n "${BINATE_BUNDLE:-}" ]; then
    BUNDLE="$(cd "$BINATE_BUNDLE" 2>/dev/null && pwd)" || {
        echo "fetch-builder: BINATE_BUNDLE='$BINATE_BUNDLE' is not a directory" >&2
        exit 1; }
    [ -d "$BUNDLE/bin" ] && [ -d "$BUNDLE/lib" ] || {
        echo "fetch-builder: BINATE_BUNDLE='$BUNDLE' is not a bundle (need bin/ and lib/)" >&2
        exit 1; }
    if [ "$mode" = lib ]; then
        echo "$BUNDLE/lib"
    else
        [ -x "$BUNDLE/bin/$tool" ] || {
            echo "fetch-builder: BINATE_BUNDLE='$BUNDLE' has no executable bin/$tool" >&2
            exit 1; }
        echo "$BUNDLE/bin/$tool"
    fi
    exit 0
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

VERSION="${BUILDER_VERSION:-}"
if [ -z "$VERSION" ]; then
    [ -f "$REPO_DIR/BUILDER_VERSION" ] || {
        echo "fetch-builder: no BUILDER_VERSION env var or file" >&2; exit 1; }
    VERSION="$(tr -d '[:space:]' < "$REPO_DIR/BUILDER_VERSION")"
fi

# Resolve 'latest' to the newest published bnc-* release tag.
if [ "$VERSION" = latest ]; then
    api="${BINATE_API_URL:-https://api.github.com/repos/binate/binate}/releases/latest"
    VERSION="$(curl -fsSL "$api" \
        | sed -n 's/.*"tag_name": *"\(bnc-[^"]*\)".*/\1/p' | head -1)"
    [ -n "$VERSION" ] || {
        echo "fetch-builder: could not resolve the latest bnc-* release" >&2; exit 1; }
fi

case "$VERSION" in
    bnc-*) ;;
    *) echo "fetch-builder: unsupported BUILDER_VERSION '$VERSION'" >&2
       echo "fetch-builder: expected bnc-X.Y.Z or 'latest'" >&2; exit 1 ;;
esac

case "$(uname -s)" in
    Darwin) OS=macos ;;
    Linux)  OS=linux ;;
    *)      OS="$(uname -s | tr '[:upper:]' '[:lower:]')" ;;
esac
case "$(uname -m)" in
    arm64|aarch64) ARCH=arm64 ;;
    x86_64|amd64)  ARCH=x64 ;;
    *)             ARCH="$(uname -m)" ;;
esac

CACHE="${BINATE_CACHE_DIR:-$HOME/.cache/binate/builders}/$VERSION/$OS-$ARCH"
BUNDLE="$CACHE/bundle"
MARKER="$BUNDLE/.fetched"

if [ ! -f "$MARKER" ]; then
    asset="$VERSION-$OS-$ARCH.tar.gz"
    base="${BINATE_RELEASE_URL:-https://github.com/binate/binate/releases/download}/$VERSION"
    tmp="$(mktemp -d)"
    trap 'rm -rf "$tmp"' EXIT INT TERM
    curl -fsSL --retry 3 --retry-delay 2 -o "$tmp/SHA256SUMS" "$base/SHA256SUMS" || {
        echo "fetch-builder: failed to download $base/SHA256SUMS" >&2; exit 1; }
    want="$(awk -v a="$asset" '$2 == a { print $1; exit }' "$tmp/SHA256SUMS")"
    [ -n "$want" ] || {
        echo "fetch-builder: no SHA256SUMS entry for $asset in $VERSION" >&2; exit 1; }
    curl -fsSL --retry 3 --retry-delay 2 -o "$tmp/$asset" "$base/$asset" || {
        echo "fetch-builder: failed to download $base/$asset" >&2; exit 1; }
    if command -v sha256sum >/dev/null 2>&1; then
        got="$(sha256sum "$tmp/$asset" | awk '{print $1}')"
    else
        got="$(shasum -a 256 "$tmp/$asset" | awk '{print $1}')"
    fi
    [ "$got" = "$want" ] || {
        echo "fetch-builder: sha256 mismatch for $asset" >&2
        echo "  expected $want" >&2; echo "  actual   $got" >&2; exit 1; }
    # The tarball has a single top-level dir; strip it for a stable layout.
    rm -rf "$BUNDLE"; mkdir -p "$BUNDLE"
    tar -xzf "$tmp/$asset" -C "$BUNDLE" --strip-components=1
    touch "$MARKER"
fi

if [ "$mode" = lib ]; then
    echo "$BUNDLE/lib"
else
    echo "$BUNDLE/bin/$tool"
fi
