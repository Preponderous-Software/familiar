Familiar
========

One Halloween night in a New England hill town. You are Wren, twelve, out
trick-or-treating, helping your grandmother keep Samhain, and tonight you
can turn into a cat.

Play
----

Windows:  double-click familiar.exe. Windows may say it protected your PC,
          because the game is not signed: choose "More info", then
          "Run anyway".
Linux:    ./familiar   (needs OpenGL 4.5, which any current Mesa or vendor
          driver has, and glibc 2.34 or newer: Ubuntu 22.04, Debian 12,
          Fedora 35 and later)
Browser:  the web download is the site itself (index.html, familiar.js,
          familiar.wasm). Serve the folder over http, for example
          "python3 -m http.server" inside it, and open the address it
          prints; opening index.html straight from disk does not work.

Press F1 in the game for the controls. Click in the window to look around
with the mouse; Esc frees the mouse.

  familiar --seed N    another night in the same town
  familiar --tour      watch a fixed walk through town into the night

More: https://github.com/Preponderous-Software/familiar
Play in a browser: https://familiar.play.danielstephenson.dev

Familiar is MIT-licensed (LICENSE.txt). It is built on the Strata engine
(STRATA-LICENSE.txt); THIRD-PARTY.md lists what Strata links in, and their
licences.
