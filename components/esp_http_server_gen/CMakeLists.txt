# Embed server data into the final binary

set(SERVER_DATA_DIR    ${PROJECT_DIR}/srv)
set(GENERATED_CODE_DIR ${PROJECT_DIR}/generated)

execute_process(COMMAND find ${SERVER_DATA_DIR} -type f -printf "%p "
                    RESULT_VARIABLE FIND_RESULT
                    OUTPUT_VARIABLE SRV_FILES_LIST)
                    
if( NOT FIND_RESULT EQUAL "0")
    message(FATAL_ERROR "command error: find ${SERVER_DATA_DIR}")
endif()

spaces2list(SRV_FILES_LIST)


execute_process(COMMAND ${COMPONENT_PATH}/gen_includes.sh ${SERVER_DATA_DIR} ${GENERATED_CODE_DIR}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    RESULT_VARIABLE GEN_RESULT)

if( NOT GEN_RESULT EQUAL "0")
    message(FATAL_ERROR "error: ${COMPONENT_PATH}/gen_includes.sh ${SERVER_DATA_DIR} ${GENERATED_CODE_DIR}")
endif()

idf_component_register(
	REQUIRES "esp_http_server_utils"
	SRC_DIRS "${GENERATED_CODE_DIR}"
	INCLUDE_DIRS ${COMPONENT_PATH}/include ${GENERATED_CODE_DIR}
	EMBED_FILES  ${SRV_FILES_LIST}
)