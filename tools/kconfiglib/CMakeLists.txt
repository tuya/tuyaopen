##
# @file kconfiglib/CMakeLists.txt
# @brief 
# @author Tuya
# @version 1.0.0
# @date 2023-04-08
#/

set(KCONFIG_TOOLS ${TOP_SOURCE_DIR}/tools/kconfiglib)

add_custom_command(
    OUTPUT
    show_config_prompt

    COMMAND
    ${CMAKE_COMMAND} -E echo "        ${Cyan}[make menuconfig]${ColourReset} - Modify the configuration."

    COMMAND
    ${CMAKE_COMMAND} -E echo "        ${Cyan}[make pre_config]${ColourReset} - Copy pre-config to default dot-config."

    COMMAND
    ${CMAKE_COMMAND} -E echo "        ${Cyan}[make save_config]${ColourReset} - Save dot-config to directory pre-config."

    COMMAND
    ${CMAKE_COMMAND} -E echo "        *${Cyan}[make config]${ColourReset} - Translate the dot-config to header-file. [Default Run]"

    COMMENT
    "[CONFIG] You can use other commands about [config]:"
    )


########################################
# First Run
########################################
if(NOT EXISTS "${DOT_CONFIG_DIR}/${DOT_CONFIG}")  # copy pre-config to dot-config
    message(STATUS "[KCONFIG] Copy pre-config to dot-config.")
    execute_process(
        COMMAND
        ${CMAKE_COMMAND} -E make_directory ${DOT_CONFIG_DIR}
        )
    execute_process(
        COMMAND
        ${CMAKE_COMMAND} -E copy ${PLATFORM_PATH}/${DEFAULT_CONFIG_NAME} ${DOT_CONFIG_DIR}/${DOT_CONFIG}
        )
    execute_process(
        COMMAND
        bash "${KCONFIG_TOOLS}/append_app_default_config.sh" "${APP_DEFAULT_CONFIG}" "${DOT_CONFIG_DIR}/${DOT_CONFIG}"
        )
    execute_process(
        COMMAND
        python3 "${KCONFIG_TOOLS}/set_catalog_config.py" "-b" "${PLATFORM_PATH}"
        "-s" "${TOP_SOURCE_DIR}/src" "-a" "${TOS_PROJECT_ROOT}" "-o" "${KCONFIG_CATALOG}"
        )
endif()

if(NOT EXISTS "${DOT_CONFIG_DIR}/${CMAKE_CONFIG}")  # use dot-config to generate cmake-config
    message(STATUS "[KCONFIG] Use dot-config to generate cmake-config.")
    execute_process(
        COMMAND
        python3 ${KCONFIG_TOOLS}/conf2cmake.py -c "${DOT_CONFIG_DIR}/${DOT_CONFIG}" -o "${DOT_CONFIG_DIR}/${CMAKE_CONFIG}"
        )
endif()

if(NOT EXISTS "${HEADER_DIR}/${HEADER_FILE}")  # use dot-config to generate header-file
    message(STATUS "[KCONFIG] Use dot-config to generate header-file.")
    execute_process(
        COMMAND
        ${CMAKE_COMMAND} -E make_directory ${HEADER_DIR}

        COMMAND
        python3 ${KCONFIG_TOOLS}/conf2h.py -c "${DOT_CONFIG_DIR}/${DOT_CONFIG}" -o "${HEADER_DIR}/${HEADER_FILE}" -i "${HEADER_IN_PATH}"
        )
endif()


########################################
# config
########################################
add_custom_target(config
    DEPENDS
    gen_header_file
    mkdir_header_dir

    COMMENT
    "[KCONFIG] Done."
    )
add_custom_command(
    OUTPUT
    mkdir_header_dir

    COMMAND
    ${CMAKE_COMMAND} -E make_directory ${HEADER_DIR}

    COMMENT
    "[KCONFIG] Make directory [${HEADER_DIR}]"
    )
add_custom_command(
    OUTPUT
    gen_header_file

    WORKING_DIRECTORY
    ${HEADER_DIR}

    COMMAND
    python3 ${KCONFIG_TOOLS}/conf2h.py -c "${DOT_CONFIG_DIR}/${DOT_CONFIG}" -o "${HEADER_FILE}" -i "${HEADER_IN_PATH}"

    # Don't need clean
    # BYPRODUCTS
    # ${HEADER_DIR}/${HEADER_FILE}

    COMMENT
    "[KCONFIG] Generate header file [${HEADER_FILE}]"
    )


########################################
# menuconfig
########################################
add_custom_target(menuconfig
    DEPENDS
    gen_dot_config
    gen_header_file
    conf2cmake
    show_config_prompt
    mkdir_dot_config_dir

    COMMENT
    "[KCONFIG] Please re-build project with command ${Cyan}[cmake ..]${ColourReset}."
    )
add_custom_command(
    OUTPUT
    mkdir_dot_config_dir

    COMMAND
    ${CMAKE_COMMAND} -E make_directory ${DOT_CONFIG_DIR}

    COMMENT
    "[KCONFIG] Make directory [${DOT_CONFIG_DIR}]"
    )
add_custom_command(
    OUTPUT
    gen_dot_config

    DEPENDS
    mkdir_dot_config_dir

    WORKING_DIRECTORY
    ${DOT_CONFIG_DIR}

    COMMAND
    bash ${KCONFIG_TOOLS}/run_menuconfig.sh ${KCONFIG_CATALOG} ${DOT_CONFIG}

    # Don't need clean
    # BYPRODUCTS
    # ${DOT_CONFIG_DIR}/${DOT_CONFIG}
    # ${DOT_CONFIG_DIR}/${DOT_CONFIG}.old

    COMMENT
    "[KCONFIG] Generate dot config [${DOT_CONFIG}]"
    )
add_custom_command(
    OUTPUT
    conf2cmake

    WORKING_DIRECTORY
    ${DOT_CONFIG_DIR}

    COMMAND
    python3 ${KCONFIG_TOOLS}/conf2cmake.py -c "${DOT_CONFIG_DIR}/${DOT_CONFIG}" -o "${CMAKE_CONFIG}"

    # Don't need clean
    # BYPRODUCTS
    # ${DOT_CONFIG_DIR}/${CMAKE_CONFIG}

    COMMENT
    "[KCONFIG] Generate cmake config [${CMAKE_CONFIG}]"
    )


########################################
# pre config
########################################
add_custom_target(pre_config
    DEPENDS
    conf2cmake
    gen_header_file
    show_config_prompt
    choice_pre_config

    COMMENT
    "[KCONFIG] Please re-build project with command ${Cyan}[cmake ..]${ColourReset}."
    )
add_custom_command(
    OUTPUT
    choice_pre_config

    COMMAND
    bash ${KCONFIG_TOOLS}/choice_pre_config.sh "${PRE_CONFIG_DIR}" "${DOT_CONFIG_DIR}/${DOT_CONFIG}" "${CONFIG_CMAKE}"
    )


########################################
# save config
########################################
add_custom_target(save_config
    COMMAND
    bash ${KCONFIG_TOOLS}/save_config.sh "${DOT_CONFIG_DIR}/${DOT_CONFIG}" "${PRE_CONFIG_DIR}"
    )


if(EXISTS ${DOT_CONFIG_DIR}/${CMAKE_CONFIG})
    include(${DOT_CONFIG_DIR}/${CMAKE_CONFIG})
else()
    message(WARNING "
    [KCONFIG] Don't found
    [${DOT_CONFIG_DIR}/${CMAKE_CONFIG}]
    Please use command: [make menuconfig].
    ")
endif()

