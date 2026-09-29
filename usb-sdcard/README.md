# USB / SD Card Folder

This `apps/` folder is a ready-to-copy Homebrew Channel layout - one subfolder per game, each
already containing a `meta.xml` (so the game shows up with a name and description in the
Homebrew Channel). What's still missing in each subfolder is the compiled game itself
(`boot.dol`), because that has to be built with devkitPro first (see the main `README.md`).

## How to finish this and use it

1. Build a game (`cd games/01-pointer-shooter && make`), or download its `.dol` from a
   GitHub Actions run.
2. Rename the resulting `.dol` file to `boot.dol` and copy it into the matching folder here,
   e.g. `usb-sdcard/apps/01-pointer-shooter/boot.dol`.
3. Repeat for as many games as you want on the device.
4. Copy the whole `apps` folder to the root of your SD card or USB drive (so it becomes
   `SD:/apps/...` or `USB:/apps/...`).
5. Insert it into the Wii and open the Homebrew Channel - every game you added a `boot.dol`
   for will appear in the list with its name and description.

You do not need to add every game - only copy over the subfolders you actually built.
