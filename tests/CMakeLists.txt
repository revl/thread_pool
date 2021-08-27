add_library(catch2 catch_main.cpp)

set(UNIT_TESTS
	trivial_test
)

foreach(TEST_NAME IN LISTS UNIT_TESTS)
	add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
	target_link_libraries(${TEST_NAME} ${PROJECT_NAME} catch2)
	add_test(${TEST_NAME} ${TEST_NAME})
endforeach(TEST_NAME)

if(BUILD_COVERAGE)
	setup_target_for_coverage_lcov(
		NAME coverage
		EXECUTABLE ctest -j ${PROCESSOR_COUNT}
		DEPENDENCIES ${UNIT_TESTS}
		EXCLUDE "/usr/*"
			"${PROJECT_SOURCE_DIR}/tests/catch.hpp"
	)
endif()
