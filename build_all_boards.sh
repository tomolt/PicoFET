#!/bin/sh

if [ $# -ne 1 ]; then
	printf "usage: %s <version>\n" "$0" >&2
	exit 1
fi

version="$1"

mkdir buildx
mkdir buildx/images

for board in $(cat supported_boards.txt); do
	printf "BUILDING IMAGE FOR BOARD '%s'\n" $board
	bdir=buildx/$board
	mkdir -p $bdir
	cmake -B $bdir -DCMAKE_BUILD_TYPE=Release -DPICO_BOARD=$board
	make -C $bdir $MAKEFLAGS
	picotool uf2 convert $bdir/PicoFET.elf "buildx/images/PicoFET_${version}_${board}.uf2"
done
