# This file contains macros dealing with translation
# files for Quassel IRC.

# Copyright (C) 2008 by the Quassel Project, devel@quassel-irc.org
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


macro(quassel_generate_qm outvar basename)
  set(input ${CMAKE_SOURCE_DIR}/i18n/${basename}.ts)
  set(output ${CMAKE_CURRENT_BINARY_DIR}/${basename}.qm)
  add_custom_command(OUTPUT ${output}
          COMMAND ${QT_LRELEASE_EXECUTABLE}
          ARGS ${input}
               -qm ${output}
               -silent -compress
          DEPENDS ${CMAKE_SOURCE_DIR}/i18n/${basename}.ts)
  set(${outvar} ${output})
endmacro(quassel_generate_qm outvar basename)

macro(quassel_generate_i18n_resource outvar)
  set(linguas ${ARGN})
  if(QT_LRELEASE_EXECUTABLE)
    # We always include quassel.ts
    quassel_generate_qm(QM quassel)
    set(outfiles ${QM})

    # Find more languages
    file(GLOB avail_tsfiles ${CMAKE_SOURCE_DIR}/i18n/quassel_*.ts)
    foreach(TS_FILE ${avail_tsfiles})
      get_filename_component(basename ${TS_FILE} NAME_WE)
      string(REGEX REPLACE "quassel_(.+)$" "\\1" lang ${basename})
      # test if we want this
      set(flg 1)
      if(linguas)
        string(REGEX MATCH "${lang}" flg ${linguas})
      endif(linguas)
      if(flg)
        quassel_generate_qm(QM ${basename})
        set(outfiles ${outfiles} ${QM})
        set(gen_linguas "${gen_linguas} ${lang}")
      endif(flg)
    endforeach(TS_FILE ${avail_tsfiles})

    # Write resource file
    set(resfile ${CMAKE_CURRENT_BINARY_DIR}/i18n.qrc)
    file(WRITE ${resfile} "<!DOCTYPE RCC><RCC version=\"1.0\">\n"
                          "<qresource prefix=\"/i18n\">\n")
    foreach(file ${outfiles})
      get_filename_component(file ${file} NAME)
      file(APPEND ${resfile} "    <file>${file}</file>\n")
    endforeach(file ${outfiles})
    file(APPEND ${resfile} "</qresource>\n</RCC>\n")
    add_custom_command(OUTPUT ${resfile} DEPENDS ${outfiles})
    #set_directory_properties(PROPERTIES
    #      ADDITIONAL_MAKE_CLEAN_FILES "${outfiles} i18n.qrc")

    # Generate resource
    qt4_add_resources(RC_OUT ${resfile})
    set(${outvar} ${RC_OUT})

    message(STATUS "Including languages:${gen_linguas}")
  else(QT_LRELEASE_EXECUTABLE)
    message(STATUS "WARNING: lrelease not found, you won't have translations!")
  endif(QT_LRELEASE_EXECUTABLE)
endmacro(quassel_generate_i18n_resource outvar)

