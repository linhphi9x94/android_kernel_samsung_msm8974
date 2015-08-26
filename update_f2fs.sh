#!/bin/bash
git fetch f2fs

find . -not -path '*/\.*' -name '*f2fs*' | grep -v 'update_f2fs\|recovery\|ramdisk' | while read f2fs; do rm -rf $f2fs; git checkout 6282adbf932c226f76e1b83e074448c79976fe75 -- $f2fs; done

rm -rf fs/f2fs
git checkout 6282adbf932c226f76e1b83e074448c79976fe75 -- fs/f2fs

git cherry-pick --no-commit ea57fcdbc82dfc8cf085a7afd93fa47534f85078
git cherry-pick --no-commit 46f88bda5328b543775dcfe32fddd629c9bbb2d0
git cherry-pick --no-commit 389ca34d538e3978eadb9bc98b59f62708eec686
git status
