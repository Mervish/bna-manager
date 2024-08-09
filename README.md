# BNAGUI
GUI tool for exploring internal structure of imas1 BNA files and extacting/packing data from them. Supports 2-level extraction for certain formats: i.e. you can extact(or inject) string from the SCB-file contained in BNA without extracting SCB-file itself.

# BNAMaster
Jack of all trades utility for operating with BNA-files on masse. Currently retooled to create script files.

# imaspatcher
Extracts data or injects it back to the game. Works with script files made by BNAMaster.

# Command line tools
[formatname]tool. Dedicated command-line tools for extraction of various formats. Low priority, since batch work would be done by imaspatcher. Some of them broken.

# Build
Uses C++23, Qt6 and Boost 1.81.

# Supported formats
- SCB: Can unpack/repack unicode from the .scb's internal MSG-section. Used to replace (translate) game's spoken text.
- BXR: Can convert .bxr files to their (assumed) .xml origin.
- NUT: Converts game's NUT-textures to DDS and back. Since the format is an archive, it creates directory and extracts into it.
