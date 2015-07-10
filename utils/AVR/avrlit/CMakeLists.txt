if (WIN32 AND NOT CYGWIN)
  # The script needs suffix.py for multiprocess to find a main module.
  set(suffix .py)
endif ()

set(llvm_avrlit_path ${LLVM_RUNTIME_OUTPUT_INTDIR}/llvm-avrlit${suffix})

if(NOT "${CMAKE_CFG_INTDIR}" STREQUAL ".")
  foreach(BUILD_MODE ${CMAKE_CONFIGURATION_TYPES})
    string(REPLACE ${CMAKE_CFG_INTDIR} ${BUILD_MODE} bi ${llvm_avrlit_path})
    configure_file(
      llvm-avrlit.in
      ${bi}
      )
  endforeach()
else()
  set(BUILD_MODE .)
  configure_file(
    llvm-avrlit.in
    ${llvm_avrlit_path}
    )
endif()
