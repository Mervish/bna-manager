# BNATool
Extractor/packer for the .bna archives used in the Namco game THE iDOLM@STER.

# BNAMaster
Extractor/packer for the game's spoken text. Gathers text from the game into the .json file. And can apply text from the .json file to the game.

# Build
Requires Qt6 and Boost. Uses C++20.

# Features
- Complete extraction of .bna contents to and packing from the folders.
- Extraction/replacement of files in .bna archives 
- On-the-fly conversion of supported filetypes directly from the .bna archive without the need to unpack the files.

# Supported formats
- SCB: Can unpack/repack unicode from the .scb's internal MSG-section. Used to replace (translate) game's spoken text.
- BXR: Can convert .bxr files to their (assumed) .xml origin.