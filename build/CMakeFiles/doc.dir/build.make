# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.24

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/riley/Desktop/wrench

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/riley/Desktop/wrench/build

# Utility rule file for doc.

# Include any custom commands dependencies for this target.
include CMakeFiles/doc.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/doc.dir/progress.make

CMakeFiles/doc: libwrench.a

doc: CMakeFiles/doc
doc: CMakeFiles/doc.dir/build.make
	python3 /home/riley/Desktop/wrench/doc/scripts/generate_rst.py /home/riley/Desktop/wrench/docs/2.2-dev /home/riley/Desktop/wrench/doc/source 2.2-dev
	sphinx-build /home/riley/Desktop/wrench/doc/source /home/riley/Desktop/wrench/docs/build/2.2-dev
	cp -R /home/riley/Desktop/wrench/docs/build/2.2-dev /home/riley/Desktop/wrench/docs/build/latest
.PHONY : doc

# Rule to build all files generated by this target.
CMakeFiles/doc.dir/build: doc
.PHONY : CMakeFiles/doc.dir/build

CMakeFiles/doc.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/doc.dir/cmake_clean.cmake
.PHONY : CMakeFiles/doc.dir/clean

CMakeFiles/doc.dir/depend:
	cd /home/riley/Desktop/wrench/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/riley/Desktop/wrench /home/riley/Desktop/wrench /home/riley/Desktop/wrench/build /home/riley/Desktop/wrench/build /home/riley/Desktop/wrench/build/CMakeFiles/doc.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/doc.dir/depend

