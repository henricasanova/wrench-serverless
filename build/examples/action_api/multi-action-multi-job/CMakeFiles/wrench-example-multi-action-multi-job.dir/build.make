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

# Include any dependencies generated for this target.
include examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/compiler_depend.make

# Include the progress variables for this target.
include examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/progress.make

# Include the compile flags for this target's objects.
include examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/flags.make

examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJobController.cpp.o: examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/flags.make
examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJobController.cpp.o: /home/riley/Desktop/wrench/examples/action_api/multi-action-multi-job/MultiActionMultiJobController.cpp
examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJobController.cpp.o: examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/riley/Desktop/wrench/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJobController.cpp.o"
	cd /home/riley/Desktop/wrench/build/examples/action_api/multi-action-multi-job && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJobController.cpp.o -MF CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJobController.cpp.o.d -o CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJobController.cpp.o -c /home/riley/Desktop/wrench/examples/action_api/multi-action-multi-job/MultiActionMultiJobController.cpp

examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJobController.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJobController.cpp.i"
	cd /home/riley/Desktop/wrench/build/examples/action_api/multi-action-multi-job && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/riley/Desktop/wrench/examples/action_api/multi-action-multi-job/MultiActionMultiJobController.cpp > CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJobController.cpp.i

examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJobController.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJobController.cpp.s"
	cd /home/riley/Desktop/wrench/build/examples/action_api/multi-action-multi-job && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/riley/Desktop/wrench/examples/action_api/multi-action-multi-job/MultiActionMultiJobController.cpp -o CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJobController.cpp.s

examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJob.cpp.o: examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/flags.make
examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJob.cpp.o: /home/riley/Desktop/wrench/examples/action_api/multi-action-multi-job/MultiActionMultiJob.cpp
examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJob.cpp.o: examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/riley/Desktop/wrench/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJob.cpp.o"
	cd /home/riley/Desktop/wrench/build/examples/action_api/multi-action-multi-job && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJob.cpp.o -MF CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJob.cpp.o.d -o CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJob.cpp.o -c /home/riley/Desktop/wrench/examples/action_api/multi-action-multi-job/MultiActionMultiJob.cpp

examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJob.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJob.cpp.i"
	cd /home/riley/Desktop/wrench/build/examples/action_api/multi-action-multi-job && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/riley/Desktop/wrench/examples/action_api/multi-action-multi-job/MultiActionMultiJob.cpp > CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJob.cpp.i

examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJob.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJob.cpp.s"
	cd /home/riley/Desktop/wrench/build/examples/action_api/multi-action-multi-job && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/riley/Desktop/wrench/examples/action_api/multi-action-multi-job/MultiActionMultiJob.cpp -o CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJob.cpp.s

# Object files for target wrench-example-multi-action-multi-job
wrench__example__multi__action__multi__job_OBJECTS = \
"CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJobController.cpp.o" \
"CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJob.cpp.o"

# External object files for target wrench-example-multi-action-multi-job
wrench__example__multi__action__multi__job_EXTERNAL_OBJECTS =

examples/action_api/multi-action-multi-job/wrench-example-multi-action-multi-job: examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJobController.cpp.o
examples/action_api/multi-action-multi-job/wrench-example-multi-action-multi-job: examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/MultiActionMultiJob.cpp.o
examples/action_api/multi-action-multi-job/wrench-example-multi-action-multi-job: examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/build.make
examples/action_api/multi-action-multi-job/wrench-example-multi-action-multi-job: libwrench.a
examples/action_api/multi-action-multi-job/wrench-example-multi-action-multi-job: /usr/local/lib/libsimgrid.so
examples/action_api/multi-action-multi-job/wrench-example-multi-action-multi-job: examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/riley/Desktop/wrench/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable wrench-example-multi-action-multi-job"
	cd /home/riley/Desktop/wrench/build/examples/action_api/multi-action-multi-job && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/wrench-example-multi-action-multi-job.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/build: examples/action_api/multi-action-multi-job/wrench-example-multi-action-multi-job
.PHONY : examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/build

examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/clean:
	cd /home/riley/Desktop/wrench/build/examples/action_api/multi-action-multi-job && $(CMAKE_COMMAND) -P CMakeFiles/wrench-example-multi-action-multi-job.dir/cmake_clean.cmake
.PHONY : examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/clean

examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/depend:
	cd /home/riley/Desktop/wrench/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/riley/Desktop/wrench /home/riley/Desktop/wrench/examples/action_api/multi-action-multi-job /home/riley/Desktop/wrench/build /home/riley/Desktop/wrench/build/examples/action_api/multi-action-multi-job /home/riley/Desktop/wrench/build/examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : examples/action_api/multi-action-multi-job/CMakeFiles/wrench-example-multi-action-multi-job.dir/depend

