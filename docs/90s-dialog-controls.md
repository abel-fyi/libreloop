# Shared dialog controls

The custom file chooser, rename/value/folder dialogs, MIDI settings and Help now share the editor skin: inset light-gray text wells, consistent title/close controls, flat file rows and a small yellow folder glyph shared with Browser. File chooser input, selection, keyboard navigation and the 0.35-second double-click formula are retained. Studio Help describes Arrange/Pattern switching; the original window help remains in legacy mode.

All 137 CTest cases passed, including unchanged original chooser and audio fixtures. Theme rendering checks the field color and readable foreground at 1, 1.25, 1.5, 2 and 3x density. Real software-rendered captures under `../../baseline-artifacts/90s-dialogs/` cover rename, folder, Help, exact values, instrument choice, recording and file chooser at 1448×1086; the chooser was also inspected at 900×562. Playback capture shows continuous master/EQ spectra and Mixer levels using actual audio output. This is not physical device/latency verification.

The executable measured 3,744,320 bytes before the Help wording update (+144 bytes from the six-op checkpoint). No extra texture or dependency is added. Full software performance results are recorded separately after isolated runs. Native GPU, physical trackpad and native HiDPI remain unverified.

The subsequent user request supersedes the large blue captions: compact gray captions and dotted Arrangement/velocity surfaces are the next change. This checkpoint records shared dialog controls, not final visual acceptance.
