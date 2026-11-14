# Log

## 2026-11-13

* Bundle created with ext-pixel-queue 0.11.0: [queue](api/queue.md) (`PixelQueue`, `PixelQueueException`, 13 constants), [build](runbooks/build.md). A frame of commands under a transform and a clip, nine shape methods, `append()` for commands made elsewhere, layers, and `drawn()` / `changed()` over the last two frames. `PHP_PIXEL_QUEUE_VERSION` 0.11.0, `branch-alias` `dev-main` is `0.11.x-dev`, on the Venusian 0.11 line. `config.m4` passes `-ffp-contract=off -fno-builtin-sin -fno-builtin-cos`: the first build without the last two called `__sincos_stret` and gave a flattened ellipse's points a different last bit from separate `sin()` and `cos()`. Built with `-Wall -Wextra` with no warning from `src/`, `fmadd` count 0, installed and 115 passed on Homebrew PHP 8.4.27 NTS and ZTS (macOS arm64), NTS also under guard malloc. Not yet built on Linux.
