#!/usr/bin/env bash
set -euo pipefail
trap 's=$?; echo >&2 "$0: Error on line "$LINENO": $BASH_COMMAND"; exit $s' ERR

make -j8
cp mkb2.rel_sample.rel ~/Documents/repos/2022/CustomPack/_ROOT/files/
cp config.custompack.json ~/Documents/repos/2022/CustomPack/_ROOT/files/config.json
