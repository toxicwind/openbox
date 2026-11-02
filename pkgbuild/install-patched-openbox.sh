#!/usr/bin/env bash
# install-patched-openbox.sh — build + install the stoplight-patched openbox
# (ranch/openbox) into the keeper build dir and point agent-wm at it for :99.
set -euo pipefail
FORK=/home/toxic/pens/estate/ranch/openbox
OB=/home/toxic/.browserless/keeper/openbox-build
mkdir -p "$OB/bin" "$OB/lib"
# Ensure a fresh build with the stoplight patch (framerender.c) is compiled.
(cd "$FORK" && touch openbox/framerender.c && make -j"$(nproc)")
cp -L "$FORK"/openbox/.libs/openbox "$OB"/bin/openbox
cp -Lf "$FORK"/obrender/.libs/libobrender.so.32.* "$OB"/lib/libobrender.so.32
cp -Lf "$FORK"/obt/.libs/libobt.so.2.0.3 "$OB"/lib/libobt.so.2
echo "patched openbox installed to $OB"
