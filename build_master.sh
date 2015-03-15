#!/bin/bash
if [ "${1}" = "skip" ] ; then
	rm arter97-kernel-hlte-"$(cat version)".zip 2>/dev/null
	cp boot.img kernelzip/boot.img
	cd kernelzip/
	7z a -mx9 arter97-kernel-hlte-"$(cat ../version)"-tmp.zip *
	zipalign -v 4 arter97-kernel-hlte-"$(cat ../version)"-tmp.zip ../arter97-kernel-hlte-"$(cat ../version)".zip
	rm arter97-kernel-hlte-"$(cat ../version)"-tmp.zip
	cd ..
	ls -al arter97-kernel-hlte-"$(cat version)".zip
	exit 0
fi

./build_clean.sh
./build_kernel_hlte.sh CC='$(CROSS_COMPILE)gcc' "$@"

rm arter97-kernel-hlte-"$(cat version)".zip 2>/dev/null
cp boot.img kernelzip/boot.img
cd kernelzip/
7z a -mx9 arter97-kernel-hlte-"$(cat ../version)"-tmp.zip *
zipalign -v 4 arter97-kernel-hlte-"$(cat ../version)"-tmp.zip ../arter97-kernel-hlte-"$(cat ../version)".zip
rm arter97-kernel-hlte-"$(cat ../version)"-tmp.zip
cd ..
ls -al arter97-kernel-hlte-"$(cat version)".zip

./build_clean.sh nozip
./build_kernel_hltetmo.sh CC='$(CROSS_COMPILE)gcc' "$@"

rm arter97-kernel-hltetmo-"$(cat version)".zip 2>/dev/null
cp boot.img kernelzip/boot.img
cd kernelzip/
7z a -mx9 arter97-kernel-hltetmo-"$(cat ../version)"-tmp.zip *
zipalign -v 4 arter97-kernel-hltetmo-"$(cat ../version)"-tmp.zip ../arter97-kernel-hltetmo-"$(cat ../version)".zip
rm arter97-kernel-hltetmo-"$(cat ../version)"-tmp.zip
cd ..
ls -al arter97-kernel-hltetmo-"$(cat version)".zip
