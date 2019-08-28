# Generate translations
# The LINGUAS environment variable can be used to limit the set of supported languages

# Generate a .ts file from a .po file
function(generate_ts basename)
    set(pofile ${CMAKE_CURRENT_SOURCE_DIR}/${basename}.po)
    set(tsfile ${CMAKE_CURRENT_BINARY_DIR}/${basename}.ts)

    add_custom_command(VERBATIM
        COMMENT "Preparing translation for language '${basename}'"
        COMMAND $<TARGET_FILE:Qt5::lconvert> -i ${pofile} -target-language ${basename} -of ts -o ${tsfile}
        DEPENDS ${pofile}
        OUTPUT  ${tsfile}
    )
endfunction()

# Clear variables just in case
set(tsfiles )
set(qmfiles )
set(linguas )

if (TARGET Qt5::lconvert)
    # LINGUAS can be used to limit the included languages
    set(LINGUAS "$ENV{LINGUAS}")
    # Normalize and convert into list
    string(REGEX REPLACE "[ \t,;]+" ";" LINGUAS "${LINGUAS}")

    # We support xx.po and xx_YY.po, and additionally translations for qt using qt_xx.po or qt_xx_YY.po
    file(GLOB pofiles *.po)
    foreach(pofile ${pofiles})
        get_filename_component(basename ${pofile} NAME_WE)
        # CMake can't use MATCH to get the second catch...
        string(REGEX REPLACE "(qt_)?([a-zA-Z]+)(_.+)?$" "\\2" lang ${basename})
        # Test if we want this language
        set(idx 0)
        if(LINGUAS)
            list(FIND LINGUAS ${lang} idx)  # idx will be -1 if ${lang} is not found in LINGUAS
        endif()
        if(idx GREATER -1)
            generate_ts(${basename})
            list(APPEND tsfiles ${CMAKE_CURRENT_BINARY_DIR}/${basename}.ts)
            list(APPEND qmfiles ${CMAKE_CURRENT_BINARY_DIR}/${basename}.qm)
            list(APPEND linguas ${lang})
        endif()
    endforeach()

    if (tsfiles)
        # Synchronize the (possibly outdated) .ts files with the current source tree
        add_custom_command(VERBATIM
            COMMENT "Syncing translations"
            COMMAND $<TARGET_FILE:Qt5::lupdate> -silent ${CMAKE_SOURCE_DIR}/src -ts ${tsfiles}
            COMMAND ${CMAKE_COMMAND} -E touch tsfiles.done
            DEPENDS ${tsfiles}
            OUTPUT tsfiles.done
        )

        # Generate the final translation files (.qm) for use by Qt
        add_custom_command(VERBATIM
            COMMENT "Compressing translations"
            COMMAND $<TARGET_FILE:Qt5::lrelease> -silent ${tsfiles}
            DEPENDS tsfiles.done
            OUTPUT ${qmfiles}
        )

        # Curate the language list and give diagnostic output
        list(REMOVE_DUPLICATES linguas)
        list(SORT linguas)
        string(REPLACE ";" ", " linguas_string "${linguas}")
        message(STATUS "Enabling translations for: ${linguas_string}")
    else()
        message(STATUS "No translations enabled")
    endif()
else()
    message(WARNING "Qt Linguist Tools not found, you won't have translations!")
endif()

# Always generate translations
add_custom_target(translations ALL DEPENDS ${qmfiles})

if (EMBED_DATA)
    quassel_add_resource(I18n PREFIX i18n BASEDIR ${CMAKE_CURRENT_BINARY_DIR} PATTERNS ${qmfiles} DEPENDS translations)
else()
    install(FILES ${qmfiles} DESTINATION ${CMAKE_INSTALL_DATADIR}/quassel/translations)
endif()
