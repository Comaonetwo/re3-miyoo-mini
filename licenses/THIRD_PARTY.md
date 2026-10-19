# Third-party notices

The new project source files are MIT-licensed; see ../LICENSE.
The included bin/soft_headless-armv7-static was linked by Zig 0.17.0 against
musl, LLVM libc++, libc++abi, and libunwind. Their license notices are included
in this directory. The binary contains no librw/re3/SDL/SDK or game assets.

re3, librw and Miyoo SDL are downloaded separately by the source fetch script.
They are not included in the source archive. Consult their own licenses.
Do not treat this project's MIT license as covering those projects or GTA III.

The v0.7 static CPU Device screen binary also links pinned librw
(commit 5501c4fdc7425ff926be59369a13593bb6c81b54), MIT (aap).
See librw-LICENSE.txt. re3 itself is not linked into this binary.
The source archive supplies original adapters and build scripts only;
librw/re3 sources are fetched separately. GTA III resources in private
SD packages are supplied from the owner's game and are not MIT-licensed.

## meshoptimizer v0.25

MIT, Copyright 2016–2025 Arseny Kapoulkine. Original https://github.com/zeux/meshoptimizer/tree/v0.25. License: meshoptimizer-LICENSE.txt. Simplifier adapter adds stable source triangle IDs; other upstream APIs retain their original behavior. Used only by the disabled world-geometry research prototype.

## Current playable re3 port (v0.63)

The playable re3_cpu binary links re3 and librw; earlier probe-only statements
above describe historical binaries, not this release. re3 upstream does not
provide a conventional software license: see re3-NOTICE.txt and the pinned
upstream README. Do not describe the complete linked engine as MIT-licensed
or as an OSI-licensed open-source distribution. This project's MIT license
applies to its original port code only, not re3, GTA III or third-party code.
The dynamic audio helper uses the console's SDL runtime, which is not bundled.
Credit: the re3 team, aap/librw, OnionUI, and the listed runtime authors.
