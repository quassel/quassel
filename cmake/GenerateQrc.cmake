# Generates a .qrc file named ${QRC_FILE} with path prefix ${PREFIX}, containing
# all files matching the glob ${PATTERNS} in the current working directory.
# This script is intended to be executed using the cmake -P syntax, so the
# arguments we're interested in start at ARGV3.

set(QRC_FILE ${CMAKE_ARGV3})
set(PREFIX   ${CMAKE_ARGV4})
set(PATTERNS ${CMAKE_ARGV5})

# Find all files matching PATTERNS in the current working directory
if (PATTERNS)
    file(GLOB_RECURSE files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${PATTERNS})
endif()

# Generate a temporary file first, so we don't touch the real thing unless something changed
set(qrc_tmp ${QRC_FILE}.tmp)
file(WRITE ${qrc_tmp} "<!DOCTYPE RCC>\n<RCC version=\"1.0\">\n<qresource prefix=\"/${PREFIX}\">\n")
foreach(file ${files})
    # Record the timestamp of last modification so changes are detected
    file(TIMESTAMP ${file} timestamp)
    file(APPEND ${qrc_tmp} "    <file timestamp=\"${timestamp}\">${file}</file>\n")
endforeach()
file(APPEND ${qrc_tmp} "</qresource>\n</RCC>\n")

# Check if the newly generated file has the same contents (including timestamps) as the existing one.
# If the files are the same, don't touch the original to avoid useless rebuilds.
if (EXISTS ${QRC_FILE})
    file(MD5 ${QRC_FILE} orig_sum)
    file(MD5 ${qrc_tmp}  tmp_sum)
    if (NOT orig_sum STREQUAL tmp_sum)
        file(RENAME ${qrc_tmp} ${QRC_FILE})
    else()
        file(REMOVE ${qrc_tmp})
    endif()
else()
    file(RENAME ${qrc_tmp} ${QRC_FILE})
endif()
