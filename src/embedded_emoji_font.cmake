cmake_minimum_required(VERSION 3.14)

# Check if the font file exists
if(NOT EXISTS ${EMOJI_FONT_PATH})
    message(FATAL_ERROR "Font file not found: ${EMOJI_FONT_PATH}")
endif()

# Read the font file as binary
file(READ ${EMOJI_FONT_PATH} FONT_DATA HEX)

# Convert the hex data to a C array
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " FONT_DATA_ARRAY ${FONT_DATA})
string(REGEX REPLACE ", $" "" FONT_DATA_ARRAY ${FONT_DATA_ARRAY})

# Get the file size
file(SIZE ${EMOJI_FONT_PATH} FONT_SIZE)

# Create the C file with the embedded font data
file(WRITE ${EMOJI_FONT_FILE} "#include <stdint.h>\n\n")
file(APPEND ${EMOJI_FONT_FILE} "// Embedded Noto Emoji font data from: ${EMOJI_FONT_PATH}\n")
file(APPEND ${EMOJI_FONT_FILE} "const uint8_t EMBEDDED_EMOJI_FONT_DATA[] = {\n")
file(APPEND ${EMOJI_FONT_FILE} "    ${FONT_DATA_ARRAY}\n")
file(APPEND ${EMOJI_FONT_FILE} "};\n\n")
file(APPEND ${EMOJI_FONT_FILE} "const size_t EMBEDDED_EMOJI_FONT_SIZE = ${FONT_SIZE};\n")

message(STATUS "Generated embedded font data (${FONT_SIZE} bytes) from ${EMOJI_FONT_PATH}")
