# Generate and add translations
# The LINGUAS variable can be used to limit that set

if(QT_LRELEASE_EXECUTABLE)
  # We always include quassel.ts
  generate_qm(QM quassel)
  set(qm_files ${QM})

  # Find more languages
  file(GLOB avail_tsfiles quassel_*.ts)
  foreach(TS_FILE ${avail_tsfiles})
    get_filename_component(basename ${TS_FILE} NAME_WE)
    string(REGEX REPLACE "quassel_([a-zA-Z]+)(_.+)?$" "\\1" lang ${basename})
    # test if we want this
    set(flg 1)
    if(LINGUAS)
      string(REGEX MATCH "${lang}" flg ${LINGUAS})
    endif(LINGUAS)
    if(flg)
      generate_qm(QM ${basename})
      set(qm_files ${qm_files} ${QM})
      set(gen_linguas "${gen_linguas} ${lang}")
    endif(flg)
  endforeach(TS_FILE ${avail_tsfiles})
  message(STATUS "Including languages:${gen_linguas}")
else(QT_LRELEASE_EXECUTABLE)
  message(STATUS "WARNING: lrelease not found, you won't have translations!")
endif(QT_LRELEASE_EXECUTABLE)

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
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${qm_files} i18n.qrc")

if(EMBED_DATA)
  set(COMMON_RCS ${COMMON_RCS} ${resfile} PARENT_SCOPE)
else(EMBED_DATA)
  install(FILES ${qm_files} DESTINATION ${DATA_INSTALL_DIR}/quassel/translations)
endif(EMBED_DATA)

add_custom_target(i18n DEPENDS ${qm_files})
