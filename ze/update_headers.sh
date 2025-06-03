thapi_dir=$(dirname -- "$( readlink -f -- "$0"; )")
temp_dir=$(mktemp -d)
echo "Cloning https://github.com/oneapi-src/level-zero"
git clone -q https://github.com/oneapi-src/level-zero $temp_dir
cd $temp_dir
newest_tag=$(git tag | sort -V | tail -n 1)
tag=${1:-$newest_tag}
echo "Checkout tags/$tag"
git checkout -q tags/$tag
find include/ -iname "*.h" -print0 | rsync -av --files-from=- --from0 . $thapi_dir
#rm -rf $temp_dir
