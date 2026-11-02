# Changelog

Auto-backfilled from git history 2026-11-02 (real commits only).

## 2026-11-02
- chore: fold forward WIP (dirty-tree sweep 2026-11-02) (dee3380)

## 2026-10-30
- chore(eidos): apply bun-scaffold bunfig + infra drop-in env (qdrant/hindsight/kafka/redis) — eidos is the pare (6c0813e)

## 2026-10-22
- framerender: stoplight surface-bake (non-solid grad -> pixel_data_to_pixmap); GREEN disc visibly renders on a  (4c71129)
- framerender: stoplight_paint_btn draws discs into button windows; still not surfacing on :99 composite (f3f6ec1)
- framerender: stoplight discs on title pixmap + re-expose; renderer-verified draw, title compositing still elud (6b5adc8)
- framerender: add title-pixmap draw debug (23f0f39)
- framerender: draw stoplight discs onto title pixmap via imlib2 (renderer-proven, still not visibly composited  (1928b3d)

## 2026-10-21
- framerender: bake stoplight color into button surface (non-solid grad) so pixel_data_to_pixmap transfers it; e (c23838b)
- framerender: draw stoplight discs onto button pixmap via imlib2 (draw executes; needs obrender surface bake to (35f57db)
- pkgbuild: sync sha256sums to the vendored source set (029eecd)
- pkgbuild: vendor the missing Arch openbox patches (d2e75d2)
- pkgbuild: package the stoplight-framerender patch (15e9f7e)
- fork: gitignore generated build artifacts (3359c57)
- patch: render macOS stoplight buttons in framerender.c (b8c1961)
- fork: add Arch PKGBUILD build recipe for the stoplight openbox patch (ddca7e9)

## 2026-05-31
- Include X.h from screen.h for Window and Time (3a2fbc6)
