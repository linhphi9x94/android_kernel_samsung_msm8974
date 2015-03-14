#!/bin/bash
if [ "${1}" = "skip" ] ; then
	rm arter97-kernel-klte-"$(cat version)".zip 2>/dev/null
	cp boot.img kernelzip/boot.img
	cd kernelzip/
	7z a -mx9 arter97-kernel-klte-"$(cat ../version)"-tmp.zip *
	zipalign -v 4 arter97-kernel-klte-"$(cat ../version)"-tmp.zip ../arter97-kernel-klte-"$(cat ../version)".zip
	rm arter97-kernel-klte-"$(cat ../version)"-tmp.zip
	cd ..
	ls -al arter97-kernel-klte-"$(cat version)".zip
	exit 0
fi

./build_clean.sh
./build_kernel_klte.sh CC='$(CROSS_COMPILE)gcc' "$@"

rm arter97-kernel-klte-"$(cat version)".zip 2>/dev/null
cp boot.img kernelzip/boot.img
cd kernelzip/
7z a -mx9 arter97-kernel-klte-"$(cat ../version)"-tmp.zip *
zipalign -v 4 arter97-kernel-klte-"$(cat ../version)"-tmp.zip ../arter97-kernel-klte-"$(cat ../version)".zip
rm arter97-kernel-klte-"$(cat ../version)"-tmp.zip
cd ..
ls -al arter97-kernel-klte-"$(cat version)".zip

./build_clean.sh nozip
./build_kernel_kltekor.sh CC='$(CROSS_COMPILE)gcc' "$@"

rm arter97-kernel-kltekor-"$(cat version)".zip 2>/dev/null
cp boot.img kernelzip/boot.img
cd kernelzip/
7z a -mx9 arter97-kernel-kltekor-"$(cat ../version)"-tmp.zip *
zipalign -v 4 arter97-kernel-kltekor-"$(cat ../version)"-tmp.zip ../arter97-kernel-kltekor-"$(cat ../version)".zip
rm arter97-kernel-kltekor-"$(cat ../version)"-tmp.zip
cd ..
ls -al arter97-kernel-kltekor-"$(cat version)".zip
