get_funcs() {
  readelf -W --dyn-syms "$1" | awk '/GLOBAL/ && /DEFAULT/ && /FUNC/ && $8 ~ /^ze/ {print $8}' | LC_COLLATE=C sort
}

diff -u --label "$1" --label "$2" <(get_funcs $1) <(get_funcs $2)
