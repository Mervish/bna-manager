# BNAGUI
GUI tool for exploring internal structure of imas1 BNA files and extacting/packing data from them. Supports 2-level extraction for certain formats: i.e. you can extact(or inject) string from the SCB-file contained in BNA without extracting SCB-file itself.

# bnamaster
Jack of all trades utility for operating with BNA-files on masse. Currently retooled to create script files.

# imaspatcher
Extracts data or injects it back to the game. Works with script files made by BNAMaster.

# Command line tools
[formatname]tool. Dedicated command-line tools for extraction of various formats. Low priority, since batch processing by imaspatcher is more effecient.

# Build requirements
- C++23, in particular std::spanstream and std::byteswap - two critical components of the file reading. I use MinGW13.1.0 to build the project.
- Qt6, for the BNAGui.
- Boost, tested on 1.81.
- OpenXLSX, requires copying it's source codes to the project's source directory.
- pugixlm, included with OpenXLSX.

# Supported formats
- SCB: A container for the spoken text data. Unicode strings from the .scb's internal MSG section can be extracted(/injected) as CSV.
- BXR: A "binary xml" format. Can be converted to it's (assumed) .xml origin and back.
- NUT: A DDS texture archive. Can be exported as a collection of DDS-files.
