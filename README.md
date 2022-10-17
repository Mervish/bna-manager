# bna-manager
Simple extractor/packer for the .bna archives used in some Namco games. BNA is a fairly simple archive format, but it has some quirks regarding the header that I had to take into account.

# Build
Requires Qt6 and Boost.

# Current state
Project is technically ready and able to unpack/repack bna-archives.

Future plans include:
- Expanding interface, adding ability to drag and drop.
- Integrating [scbeditor](https://github.com/Mervish/scb-editor) into the manager, so it could extract strings directly from the BNA file.