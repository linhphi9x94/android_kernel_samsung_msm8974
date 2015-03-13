#!/sbin/sh
#
# /system/addon.d/97-kernel.sh
# During a CM12 upgrade, this script prevents kernel writings and backs up its dependencies.
# /system is formatted and reinstalled, then the file is restored.
#

rm /dev/block/mmcblk0p14

. /tmp/backuptool.functions

list_files() {
cat <<EOF
etc/audio_policy.conf
EOF
}

case "$1" in
  backup)
    list_files | while read FILE DUMMY; do
      backup_file $S/"$FILE"
    done
  ;;
  restore)
    list_files | while read FILE REPLACEMENT; do
      R=""
      [ -n "$REPLACEMENT" ] && R="$S/$REPLACEMENT"
      [ -f "$C/$S/$FILE" ] && restore_file $S/"$FILE" "$R"
    done
    rm -rf /system/vendor/lib/hw
  ;;
  pre-backup)
    # Stub
  ;;
  post-backup)
    # Stub
  ;;
  pre-restore)
    # Stub
  ;;
  post-restore)
    # Stub
  ;;
esac
