Getting Started                        {#getting-started}
============

@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/getting-started.html">Developer</a> - <a href="../internal/getting-started.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/getting-started.html">User</a> - <a href="../internal/getting-started.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/getting-started.html">User</a> -  <a href="../developer/getting-started.html">Developer</a></div> @endWRENCHDoc

[TOC]

The first step is to have WRENCH library installed. If it is not done yet, please 
follow the instructions to [install it](@ref install).

# Running a First Example #         {#getting-started-example}

WRENCH distribution provides an example WMS implementation (`simple-wms`) available 
in the `examples` folder. A simple installation via `make && make install` already
compiles all examples.

WRENCH provides two implementations for the `simple-wms` example: a cloud-based 
implementation `wrench-simple-wms-cloud`, and an implementation to run workflows 
in a batch system (e.g., SLURM) `wrench-simple-wms-batch`.

A successful WRENCH installation will put the examples binaries in the `/usr/local/bin` 
folder (for MacOS and most Linux distributions). To run the examples, simply use 
one of the following commands:

~~~~~~~~~~~~~{.sh}
# running the cloud-based implementation
wrench-simple-wms-cloud <PATH-TO-WRENCH-SRC-FOLDER>/examples/cloud_hosts.xml <PATH-TO-WRENCH-SRC-FOLDER>/examples/genome.dax

# running the batch-based implementation
wrench-simple-wms-batch <PATH-TO-WRENCH-SRC-FOLDER>/examples/two_hosts.xml <PATH-TO-WRENCH-SRC-FOLDER>/examples/genome.dax
~~~~~~~~~~~~~

## Understanding the Simple-WMS Examples      {#getting-started-example-simplewms}

The `simple-wms` example requires two arguments: (1) a [SimGrid virtual platform 
description file](http://simgrid.gforge.inria.fr/simgrid/3.17/doc/platform.html); and
(2) a WRENCH workflow file.

**SimGrid virtual platform description file:** 
In [SimGrid](http://simgrid.gforge.inria.fr), any study must entail the description 
of the platform on which you want to simulate your application. This file includes 
definitions of computing hosts, clusters, storage, network links and routes, etc.
A detailed description on how to build your platform description file can be found
[here](http://simgrid.gforge.inria.fr/simgrid/3.17/doc/platform.html).

**WRENCH workflow file:**
WRENCH provides native parsers for [DAX](http://workflowarchive.org) (DAG in XML) 
and [JSON](http://workflowhub.org/traces/) formats. Please, refer to the hyperlinked
texts for their respective documentations.


# Preparing the Environment #         {#getting-started-prep}

@WRENCHNotInternalDoc
## Importing WRENCH ##                {#getting-started-prep-import}

For ease of use, all WRENCH abstractions are accessed via a single 
include statement:

@WRENCHUserDoc
~~~~~~~~~~~~~{.cpp}
#include <wrench.h>
~~~~~~~~~~~~~
@endWRENCHDoc

@WRENCHDeveloperDoc 
~~~~~~~~~~~~~{.cpp}
#include <wrench-dev.h>
~~~~~~~~~~~~~

Note that `wrench-dev.h` is the only necessary include statement to be added to your 
experiment code. It includes all interfaces and services provided in `wrench.h` 
([user mode](../user/getting-started.html)), and additional interfaces to develop 
your own algorithms and services.
 
@endWRENCHDoc

## Building Your CMakeLists.txt File ##                {#getting-started-prep-cmakelists}

Below is an example of a `CMakeLists.txt` file that can be used as a starting 
template for developing a WRENCH application compiled using cmake:

~~~~~~~~~~~~~{.cmake}
cmake_minimum_required(VERSION 3.2)
message(STATUS "Cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")

project(YOUR_PROJECT_NAME)

add_definitions("-Wall -Wno-unused-variable -Wno-unused-private-field")

set(CMAKE_CXX_STANDARD 11)

# include directories for dependencies and WRENCH libraries
include_directories(src/ /usr/local/include /usr/local/include/wrench)

# source files
set(SOURCE_FILES
        src/main.cpp
        )

# test files
set(TEST_FILES
        )

# wrench library and dependencies
find_library(WRENCH_LIBRARY NAMES wrench)
find_library(SIMGRID_LIBRARY NAMES simgrid)
find_library(PUGIXML_LIBRARY NAMES pugixml)
find_library(LEMON_LIBRARY NAMES emon)
find_library(GTEST_LIBRARY NAMES gtest)

# generating the executable
add_executable(my-executable ${SOURCE_FILES})
target_link_libraries(my-executable 
                        ${WRENCH_LIBRARY} 
                        ${SIMGRID_LIBRARY} 
                        ${PUGIXML_LIBRARY} 
                        ${LEMON_LIBRARY}
                     )

install(TARGETS my-executable DESTINATION bin)

# generating unit tests
add_executable(unit_tests EXCLUDE_FROM_ALL 
                 ${SOURCE_FILES} 
                 ${TEST_FILES}
              )
target_link_libraries(unit_tests 
                        ${GTEST_LIBRARY} wrench -lpthread -lm
                     )
~~~~~~~~~~~~~

@endWRENCHDoc

@WRENCHInternalDoc

Internal developers are expected to **contribute** code to WRENCH's core components.
Please, refer to the [API Reference](./annotated.html) to find the detailed 
documentation for WRENCH functions.

In addition to the common [installation steps](./install.html), WRENCH's internal
developers are strongly encouraged to install the following dependencies/tools:

- [Google Test](https://github.com/google/googletest) - version 1.8 or higher (for running test cases)
- [Doxygen](http://www.doxygen.org) - version 1.8 or higher (for generating documentation)
    

# WRENCH Directory and File Structure #         {#getting-started-structure}

WRENCH follows a standard C++ project directory and files structure:

~~~~~~~~~~~~~{.sh}
.
+-- doc                        # Documentation files
+-- examples                   # Examples folder (includes workflows, platform files, and implementations) 
+-- include                    # WRENCH header files - .h files 
+-- src                        # WRENCH source files - .cpp files
+-- test                       # WRENCH test files
+-- tools                      # Tools for supporting documentation generation and release builds
+-- .travis.yml                # Configuration file for Travis Continuous Integration
+-- sonar-project.properties   # Configuration file for Sonar Cloud Continuous Code Quality
+-- LICENSE.md                 # WRENCH license disclaimer
+-- README.md
~~~~~~~~~~~~~

@endWRENCHDoc