# Generate and add translations
# The LINGUAS variable can be used to limit that set

if (TARGET Qt5::lconvert)
  # get environment variable for translations
  set(LINGUAS "$ENV{LINGUAS}")
  string(REGEX REPLACE "[ \t]+" \; output "${LINGUAS}")
  # Find more languages
  # We support xx.po and xx_YY.po, and additionally translations for qt using qt_xx.po or qt_xx_YY.po
  file(GLOB avail_pofiles *.po)
  foreach(PO_FILE ${avail_pofiles})
    get_filename_component(basename ${PO_FILE} NAME_WE)
    # CMake can't use MATCH to get the second catch...
    string(REGEX REPLACE "(qt_)?([a-zA-Z]+)(_.+)?$" "\\2" lang ${basename})

    # test if we want this language
    set(flg 1)
    if(LINGUAS)
      string(REGEX MATCH "${lang}" flg ${LINGUAS})
    endif()
    if(flg)
      generate_ts(QM ${basename})
      generate_qm(QM ${basename})
      list(APPEND qm_files ${QM})
      list(APPEND gen_linguas ${lang})
    endif()
  endforeach(PO_FILE ${avail_pofiles})
  if(gen_linguas)
    list(REMOVE_DUPLICATES gen_linguas)
  endif()
  message(STATUS "Including languages: ${gen_linguas}")
else()
  message(STATUS "WARNING: lconvert not found, you won't have translations!")
endif()

# For a Win32 build, we need to include Qt translations if available
if(QT_TRANSLATIONS_DIR)
  if(WIN32)
    foreach(LANG ${gen_linguas})
      file(GLOB lang_files ${QT_TRANSLATIONS_DIR}/qt_${LANG}*.qm)
      foreach(absfile ${lang_files})
        get_filename_component(filename ${absfile} NAME)
        message(STATUS "Importing ${filename}")
        configure_file(${absfile} ${CMAKE_CURRENT_BINARY_DIR}/${filename} COPYONLY)
        list(APPEND qm_files ${CMAKE_CURRENT_BINARY_DIR}/${filename})
      endforeach(absfile ${lang_files})
    endforeach(LANG ${gen_linguas})
  endif()
endif()

# Write resource file
set(resfile ${CMAKE_CURRENT_BINARY_DIR}/i18n.qrc)
file(WRITE ${resfile} "<!DOCTYPE RCC><RCC version=\"1.0\">\n"
                      "<qresource prefix=\"/i18n\">\n")
foreach(file ${qm_files})
  get_filename_component(file ${file} NAME)
  file(APPEND ${resfile} "    <file>${file}</file>\n")
endforeach(file ${qm_files})
file(APPEND ${resfile} "</qresource>\n</RCC>\n")

#add_custom_command(OUTPUT ${resfile} DEPENDS ${qm_files})
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${qm_files};i18n.qrc")

if(EMBED_DATA)
  set(COMMON_RCS ${COMMON_RCS} ${resfile} PARENT_SCOPE)
else()
  install(FILES ${qm_files} DESTINATION ${CMAKE_INSTALL_DATADIR}/quassel/translations)
endif()

add_custom_target(po DEPENDS ${qm_files})
