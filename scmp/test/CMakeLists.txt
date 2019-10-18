file(GLOB source_files *.cpp *.h)
add_executable (test_scmp ${source_files})
target_link_libraries (test_scmp LINK_PUBLIC 
	scmp
	${Boost_LIBRARIES}
	)
