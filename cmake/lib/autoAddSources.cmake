function(add_include_folder path)
	file(GLOB_RECURSE header_files
		"${path}/*.h"
		"${path}/*.hpp"
		"${path}/*.hxx"
	)
	set(headers ${headers} ${header_files} PARENT_SCOPE)
endfunction()

function(add_src_folder path)
	file(GLOB_RECURSE src_files
		"${path}/*.c"
		"${path}/*.cpp"
		"${path}/*.cxx"
	)
	set(sources ${sources} ${src_files} PARENT_SCOPE)
endfunction()
