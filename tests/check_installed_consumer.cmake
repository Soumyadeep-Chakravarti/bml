file(MAKE_DIRECTORY "${WORK}")
execute_process(COMMAND "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${WORK}/prefix"
                RESULT_VARIABLE install_result)
if (NOT install_result EQUAL 0)
  message(FATAL_ERROR "BML installation failed")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}/tests/installed_consumer"
                -B "${WORK}/build" -DCMAKE_PREFIX_PATH=${WORK}/prefix
                RESULT_VARIABLE configure_result)
if (NOT configure_result EQUAL 0)
  message(FATAL_ERROR "installed consumer configuration failed")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${WORK}/build"
                RESULT_VARIABLE build_result)
if (NOT build_result EQUAL 0)
  message(FATAL_ERROR "installed consumer build failed")
endif()
execute_process(COMMAND "${WORK}/build/bml_consumer" RESULT_VARIABLE run_result)
if (NOT run_result EQUAL 0)
  message(FATAL_ERROR "installed consumer failed")
endif()
