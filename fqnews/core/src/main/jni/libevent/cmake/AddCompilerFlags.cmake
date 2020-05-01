include(CheckCCompilerFlag)

macro(add_compiler_flags)
	foreach(flag ${ARGN})
		string(REGEX REPLACE "[-.+/:= ]" "_" _flag_esc "${flag}")

		check_c_compiler_flag("${flag}" check_c_compiler_flag_${_flag_esc})

		if (check_c_compiler_flag_${_flag_esc})
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}")
		endif()
	endforeach()
endmacro()
