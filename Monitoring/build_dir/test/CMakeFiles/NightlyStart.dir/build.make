# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The program to use to edit the cache.
CMAKE_EDIT_COMMAND = /usr/bin/ccmake

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /opt/devel/flp_prototype/hackathon/Monitoring

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /opt/devel/flp_prototype/hackathon/Monitoring/build_dir

# Utility rule file for NightlyStart.

# Include the progress variables for this target.
include test/CMakeFiles/NightlyStart.dir/progress.make

test/CMakeFiles/NightlyStart:
	cd /opt/devel/flp_prototype/hackathon/Monitoring/build_dir/test && /usr/bin/ctest -D NightlyStart

NightlyStart: test/CMakeFiles/NightlyStart
NightlyStart: test/CMakeFiles/NightlyStart.dir/build.make
.PHONY : NightlyStart

# Rule to build all files generated by this target.
test/CMakeFiles/NightlyStart.dir/build: NightlyStart
.PHONY : test/CMakeFiles/NightlyStart.dir/build

test/CMakeFiles/NightlyStart.dir/clean:
	cd /opt/devel/flp_prototype/hackathon/Monitoring/build_dir/test && $(CMAKE_COMMAND) -P CMakeFiles/NightlyStart.dir/cmake_clean.cmake
.PHONY : test/CMakeFiles/NightlyStart.dir/clean

test/CMakeFiles/NightlyStart.dir/depend:
	cd /opt/devel/flp_prototype/hackathon/Monitoring/build_dir && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /opt/devel/flp_prototype/hackathon/Monitoring /opt/devel/flp_prototype/hackathon/Monitoring/test /opt/devel/flp_prototype/hackathon/Monitoring/build_dir /opt/devel/flp_prototype/hackathon/Monitoring/build_dir/test /opt/devel/flp_prototype/hackathon/Monitoring/build_dir/test/CMakeFiles/NightlyStart.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : test/CMakeFiles/NightlyStart.dir/depend

