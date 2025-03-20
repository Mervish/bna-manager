A collection of tools made for [The iDOLM@STER translation project](https://drive.google.com/drive/folders/1o0YrM2csxbC23njmMADyGrHdIAHghO50). The tools were initially made for The iDOLM@STER for Xbox360, but they also work with The iDOLM@STER Live 4 You, since that game uses the same engine.

# BNAGUI
GUI tool for exploring internal structure of imas1/L4U BNA files and extacting/packing data from them.

# bnamaster
A tool currently used to map game's bna-structure and make .json script-files with results.

# imaspatcher
Extracts data or injects it back to the game according to script files made by BNAMaster. Better used as part of [patching environment](https://drive.google.com/drive/folders/1x8upUR42WMWdKjCGVz5Spu3JMaOeLb2V).

# Command line tools
[formatname]tool. Dedicated command-line tools for extraction of various formats. They are not very useful because imaspatcher's batch processing is much more efficient and convenient, so they are low priority from a development point of view.

# Build requirements
- C++23, in particular std::spanstream and std::byteswap - two critical components of the file reading. I use MinGW13.1.0 to build the project.
- Qt6, for the BNAGui.
- Boost, tested on 1.81.
- OpenXLSX, a submodule placed in "thirdparty" folder. Requires pulling it from github separately.
- pugixlm, currently included with OpenXLSX. Will be added as a proper submodule if OpenXLSX devs do that first.

# Supported formats
- SCB: A container for the various text data. It has an internal MSG section from which strings can be extracted as XLSX and (translated) strings can be inserted back into it later.
- NUT: A DDS texture archive. Can be exported as a collection of DDS files, and DDS files can be imported back into it. Supports partial rewrite, if you only need to rewrite particular textures in a NUT-file.
- BXR: A "binary xml" format. Can be converted to it's (assumed) .xml origin and back. The only format that actually fully converts it's content both ways.
