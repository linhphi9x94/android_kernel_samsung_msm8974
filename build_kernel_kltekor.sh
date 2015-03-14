#!/bin/bash
export KERNELDIR=`readlink -f .`
export RAMFS_SOURCE=`readlink -f $KERNELDIR/ramdisk`
export USE_SEC_FIPS_MODE=true

echo "kerneldir = $KERNELDIR"
echo "ramfs_source = $RAMFS_SOURCE"

RAMFS_TMP="/tmp/arter97-klte-ramdisk"

echo "ramfs_tmp = $RAMFS_TMP"
cd $KERNELDIR

if [ "${1}" = "skip" ] ; then
	echo "Skipping Compilation"
else
	echo "Compiling kernel"
	cp defconfig .config
scripts/configcleaner "
CONFIG_MACH_KLTE_EUR
CONFIG_MACH_KLTE_KOR
CONFIG_MACH_KLTE_SKT
CONFIG_WLAN_REGION_CODE=201
CONFIG_USE_VM_KEYBOARD_REJECT
CONFIG_TDMB
CONFIG_TDMB_VENDOR_FCI
CONFIG_TDMB_FC8050
CONFIG_TDMB_FC8080
CONFIG_TDMB_VENDOR_INC
CONFIG_TDMB_VENDOR_RAONTECH
CONFIG_TDMB_VENDOR_TELECHIPS
CONFIG_TDMB_QUALCOMM
CONFIG_TDMB_SLSI
CONFIG_TDMB_SPI
CONFIG_TDMB_EBI
CONFIG_TDMB_TSIF_SLSI
CONFIG_TDMB_TSIF_QC
CONFIG_TDMB_I2C
CONFIG_TDMB_SIMUL
CONFIG_TDMB_DEBUG
CONFIG_TDMB_ANT_DET
CONFIG_TDMB_VREG_SUPPORT
CONFIG_SEC_DEVIDE_RINGTONE_GAIN
CONFIG_SND_SOC_ES704_TEMP
CONFIG_USB_LOCK_SUPPORT_FOR_MDM
CONFIG_SENSORS_SSP_SHTC1
"
	echo '
# CONFIG_MACH_KLTE_EUR is not set
CONFIG_MACH_KLTE_KOR=y
CONFIG_MACH_KLTE_SKT=y
CONFIG_WLAN_REGION_CODE=201
# CONFIG_USE_VM_KEYBOARD_REJECT is not set
CONFIG_TDMB=y
CONFIG_TDMB_VENDOR_FCI=y
# CONFIG_TDMB_FC8050 is not set
CONFIG_TDMB_FC8080=y
# CONFIG_TDMB_VENDOR_INC is not set
# CONFIG_TDMB_VENDOR_RAONTECH is not set
# CONFIG_TDMB_VENDOR_TELECHIPS is not set
CONFIG_TDMB_QUALCOMM=y
# CONFIG_TDMB_SLSI is not set
CONFIG_TDMB_SPI=y
# CONFIG_TDMB_EBI is not set
# CONFIG_TDMB_TSIF_SLSI is not set
# CONFIG_TDMB_TSIF_QC is not set
# CONFIG_TDMB_I2C is not set
# CONFIG_TDMB_SIMUL is not set
# CONFIG_TDMB_DEBUG is not set
# CONFIG_TDMB_ANT_DET is not set
# CONFIG_TDMB_VREG_SUPPORT is not set
CONFIG_SEC_DEVIDE_RINGTONE_GAIN=y
CONFIG_SND_SOC_ES704_TEMP=y
CONFIG_USB_LOCK_SUPPORT_FOR_MDM=y
CONFIG_SENSORS_SSP_SHTC1=y
' >> .config
	make oldconfig
	make "$@" || exit 1
fi

echo "Building new ramdisk"
#remove previous ramfs files
rm -rf '$RAMFS_TMP'*
rm -rf $RAMFS_TMP
rm -rf $RAMFS_TMP.cpio
#copy ramfs files to tmp directory
cp -ax $RAMFS_SOURCE $RAMFS_TMP
cd $RAMFS_TMP

find . -name '*.sh' -exec chmod 755 {} \;

$KERNELDIR/ramdisk_fix_permissions.sh 2>/dev/null

#clear git repositories in ramfs
find . -name .git -exec rm -rf {} \;
find . -name EMPTY_DIRECTORY -exec rm -rf {} \;
cd $KERNELDIR
rm -rf $RAMFS_TMP/tmp/*

cd $RAMFS_TMP
find . | fakeroot cpio -H newc -o | lzop -9 > $RAMFS_TMP.cpio.lzo
ls -lh $RAMFS_TMP.cpio.lzo
cd $KERNELDIR

echo "Making new boot image"
gcc -w -s -pipe -O2 -o tools/dtbtool/dtbtool tools/dtbtool/dtbtool.c
tools/dtbtool/dtbtool -s 2048 -o arch/arm/boot/dt.img -p scripts/dtc/ arch/arm/boot/
gcc -w -s -pipe -O2 -Itools/libmincrypt -o tools/mkbootimg/mkbootimg tools/libmincrypt/*.c tools/mkbootimg/mkbootimg.c
tools/mkbootimg/mkbootimg --kernel $KERNELDIR/arch/arm/boot/zImage --dt $KERNELDIR/arch/arm/boot/dt.img --ramdisk $RAMFS_TMP.cpio.lzo --cmdline 'console=null androidboot.hardware=qcom user_debug=23 msm_rtb.filter=0x37 ehci-hcd.park=3 enforcing=0' --base 0x00000000 --pagesize 2048 --kernel_offset 0x00008000 --ramdisk_offset 0x02000000 --tags_offset 0x01e00000 --second_offset 0x00f00000 -o $KERNELDIR/boot.img
echo -n "SEANDROIDENFORCE" >> boot.img
if [ "${1}" = "CC=\$(CROSS_COMPILE)gcc" ] ; then
	dd if=/dev/zero bs=$((13631488-$(stat -c %s boot.img))) count=1 >> boot.img
fi

echo "done"
ls -al boot.img
echo ""
