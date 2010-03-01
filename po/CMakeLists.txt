# Generate and add translations
# The LINGUAS variable can be used to limit that set

if(QT_LCONVERT_EXECUTABLE)
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
    endif(LINGUAS)
    if(flg)
      generate_ts(QM ${basename})
      generate_qm(QM ${basename})
      list(APPEND qm_files ${QM})
      list(APPEND gen_linguas ${lang})
    endif(flg)
  endforeach(PO_FILE ${avail_pofiles})
  if(gen_linguas)
    list(REMOVE_DUPLICATES gen_linguas)
  endif(gen_linguas)
  message(STATUS "Including languages: ${gen_linguas}")
else(QT_LCONVERT_EXECUTABLE)
  message(STATUS "WARNING: lconvert not found, you won't have translations!")
endif(QT_LCONVERT_EXECUTABLE)

# For a static or win32 build, we need to include Qt translations if available
if(QT_TRANSLATIONS_DIR)
  if(STATIC OR WIN32)
    foreach(LANG ${gen_linguas})
      file(GLOB lang_files ${QT_TRANSLATIONS_DIR}/qt_${LANG}*.qm)
      foreach(absfile ${lang_files})
        get_filename_component(filename ${absfile} NAME)
        message(STATUS "Importing ${filename}")
        configure_file(${absfile} ${CMAKE_CURRENT_BINARY_DIR}/${filename} COPYONLY)
        list(APPEND qm_files ${CMAKE_CURRENT_BINARY_DIR}/${filename})
      endforeach(absfile ${lang_files})
    endforeach(LANG ${gen_linguas})
  endif(STATIC OR WIN32)
endif(QT_TRANSLATIONS_DIR)

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
else(EMBED_DATA)
  install(FILES ${qm_files} DESTINATION ${DATA_INSTALL_DIR}/quassel/translations)
endif(EMBED_DATA)

add_custom_target(po DEPENDS ${qm_files})
