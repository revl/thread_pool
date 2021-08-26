cmake_minimum_required(VERSION 3.6)

set(CMAKE_CXX_STANDARD 11)

project(thread_pool LANGUAGES CXX VERSION 0.5.0)

add_compile_options(-Wall -Wextra -Werror)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG(-Wpedantic COMPILER_SUPPORTS_WPEDANTIC)
if(COMPILER_SUPPORTS_WPEDANTIC)
	add_compile_options(-Wpedantic)
endif()

add_library(${PROJECT_NAME} src/${PROJECT_NAME}.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
	set(EXPORT_NAME ThreadPoolOnMutexes)

	set_target_properties(${PROJECT_NAME}
		PROPERTIES EXPORT_NAME ${EXPORT_NAME})

	include(GNUInstallDirs)

	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	)

	install(TARGETS ${PROJECT_NAME}
		EXPORT ${EXPORT_NAME}Targets
		DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)

	install(EXPORT ${EXPORT_NAME}Targets
		FILE ${EXPORT_NAME}Targets.cmake
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${EXPORT_NAME}
	)

	include(CMakePackageConfigHelpers)
	write_basic_package_version_file(
		${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_NAME}ConfigVersion.cmake
		VERSION ${PROJECT_VERSION}
		COMPATIBILITY SameMajorVersion
	)

	configure_package_config_file(
		${PROJECT_SOURCE_DIR}/cmake/${EXPORT_NAME}Config.cmake.in
		${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_NAME}Config.cmake
		INSTALL_DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/${EXPORT_NAME}
	)

	install(FILES
		${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_NAME}Config.cmake
		${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_NAME}ConfigVersion.cmake
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${EXPORT_NAME}
	)

	export(EXPORT ${EXPORT_NAME}Targets FILE
		${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_NAME}Targets.cmake
	)

	export(PACKAGE ${EXPORT_NAME})

	include(InstallRequiredSystemLibraries)
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
	include(CPack)
endif()
