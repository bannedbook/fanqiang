# This module defines the following variables utilizing
# git to determine the parent tag. And if found the macro
# will attempt to parse them in the github tag fomat
#
# Usful for auto-versionin in ou CMakeLists
#
#  EVENT_GIT___VERSION_FOUND - Version variables foud
#  EVENT_GIT___VERSION_MAJOR - Major version.
#  EVENT_GIT___VERSION_MINOR - Minor version
#  EVENT_GIT___VERSION_STAGE - Stage version
#
# Example usage:
#
# event_fuzzy_version_from_git()
# if (EVENT_GIT___VERSION_FOUND)
#    message("Libvent major=${EVENT_GIT___VERSION_MAJOR}")
#    message("        minor=${EVENT_GIT___VERSION_MINOR}")
#    message("        patch=${EVENT_GIT___VERSION_PATCH}")
#    message("        stage=${EVENT_GIT___VERSION_STAGE}")
# endif()

include(FindGit)

macro(event_fuzzy_version_from_git)
	set(EVENT_GIT___VERSION_FOUND FALSE)

	# set our defaults.
	set(EVENT_GIT___VERSION_MAJOR 2)
	set(EVENT_GIT___VERSION_MINOR 1)
	set(EVENT_GIT___VERSION_PATCH 8)
	set(EVENT_GIT___VERSION_STAGE "beta")

	find_package(Git)

	if (GIT_FOUND)
		execute_process(
			COMMAND
				${GIT_EXECUTABLE} describe --abbrev=0
			WORKING_DIRECTORY
				${PROJECT_SOURCE_DIR}
			RESULT_VARIABLE
				GITRET
			OUTPUT_VARIABLE
				GITVERSION)

			if (GITRET EQUAL 0)
				string(REGEX REPLACE "^release-([0-9]+)\\.([0-9]+)\\.([0-9]+)-(.*)"       "\\1" EVENT_GIT___VERSION_MAJOR ${GITVERSION})
				string(REGEX REPLACE "^release-([0-9]+)\\.([0-9]+)\\.([0-9]+)-(.*)"       "\\2" EVENT_GIT___VERSION_MINOR ${GITVERSION})
				string(REGEX REPLACE "^release-([0-9]+)\\.([0-9]+)\\.([0-9]+)-(.*)"       "\\3" EVENT_GIT___VERSION_PATCH ${GITVERSION})
				string(REGEX REPLACE "^release-([0-9]+)\\.([0-9]+)\\.([0-9]+)-([aA-zZ]+)" "\\4" EVENT_GIT___VERSION_STAGE ${GITVERSION})
			endif()
		endif()
endmacro()
