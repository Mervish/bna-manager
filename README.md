# BNAGUI
GUI tool for exploring internal structure of imas1 BNA files and extacting/packing data from them. Supports 2-level extraction for certain formats: i.e. you can extact(or inject) string from the SCB-file contained in BNA without extracting SCB-file itself.

# bnamaster
Jack of all trades utility for operating with BNA-files on masse. Currently retooled to create script files.

# imaspatcher
Extracts data or injects it back to the game. Works with script files made by BNAMaster.

# Command line tools
[formatname]tool. Dedicated command-line tools for extraction of various formats. Low priority, since batch processing by imaspatcher is more effecient.

# Build
Uses C++23, Qt6 and Boost>=1.81.

# Supported formats
- SCB: A container for the spoken text data. Unicode strings from the .scb's internal MSG section can be extracted(/injected) as CSV.
- BXR: A "binary xml" format. Can be converted to it's (assumed) .xml origin and back. Currently the only format with a hard dependancy on Qt.
- NUT: A DDS texture archive. Can be exported as a collection of DDS-files.