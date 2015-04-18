#!/bin/bash
git fetch f2fs

find . -not -path '*/\.*' -name '*f2fs*' | grep -v 'update_f2fs\|recovery\|ramdisk' | while read f2fs; do rm -rf $f2fs; git checkout f2fs/dev -- $f2fs; done

rm -rf fs/f2fs
git checkout f2fs/dev -- fs/f2fs

# Workaround for f2fs/linux-3.4 not being up-to-date with dev branch
rm 0001* 2>/dev/null
git format-patch 83dfe53c185e3554c102708c70dc1e5ff4bcac2c^..83dfe53c185e3554c102708c70dc1e5ff4bcac2c
patch -R -p1 < 0001*
git add fs/f2fs/acl.c

git log --oneline f2fs/linux-3.4 | head -1 | awk '{print $1}' | while read patch; do git cherry-pick --no-commit $patch; done
git cherry-pick --no-commit 6d9dc4b4173b64b2d060aae0e2f9d46180aba310
git cherry-pick --no-commit 4cc871a25931de58496c80ac756de8303d94122e
git cherry-pick --no-commit 9aa487ad90c38f3a27a46bca04108b0bfb11dcb2

git reset
sed -i -e 's@QSTR_INIT(".", 1)@{.len = 1, .name = "."}@g' -e 's@QSTR_INIT("..", 2)@{.len = 2, .name = ".."}@g' fs/f2fs/namei.c

git status
