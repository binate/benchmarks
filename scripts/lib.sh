# Sourced by run.sh. Provides timing + a Binate dual-backend build helper.
# The sourcing script must set REPO_DIR and SCRIPT_DIR first.

# Millisecond-resolution wall time (perl Time::HiRes is core Perl), matching
# the binate repo's perf/*.sh drivers.
now()   { perl -MTime::HiRes=time -e 'printf "%.6f", time()'; }
delta() { perl -e "printf '%.3f', $2 - $1"; }
minof()    { echo "$1" | tr ' ' '\n' | awk 'NF' | sort -n | head -1; }
medianof() { echo "$1" | tr ' ' '\n' | awk 'NF' | sort -n | awk '{a[NR]=$0} END{print a[int((NR+1)/2)]}'; }

# Resolve the pinned Binate toolchain once (honours BUILDER_VERSION / BINATE_BUNDLE
# via fetch-builder.sh, exactly like the examples repo).
binate_resolve() {
    BNC="$("$SCRIPT_DIR/fetch-builder.sh")"
    LIB="$("$SCRIPT_DIR/fetch-builder.sh" --lib)"
    BP="$(dirname "$BNC")/binate-paths"
}

# binate_build <backend> <bench-root> <cmd-path> <out-bin>
#   backend  : native | llvm
#   bench-root: the benchmark's binate/ dir (its package search root)
#   cmd-path : the runnable package dir (…/cmd/<name>)
# Both backends build at -O2; the LLVM path also passes clang -O2.
binate_build() {
    _bk="$1"; _root="$2"; _cmd="$3"; _out="$4"
    _I="$("$BP" --iface --base "$LIB" --prepend "$_root")"
    _L="$("$BP" --impl  --base "$LIB" --prepend "$_root")"
    case "$_bk" in
        native) set -- --backend native -O2 ;;
        llvm)   set -- --backend llvm -O2 --cflag -O2 ;;
        *) echo "binate_build: unknown backend '$_bk'" >&2; return 2 ;;
    esac
    "$BNC" "$@" -I "$_I" -L "$_L" -o "$_out" "$_cmd"
}
