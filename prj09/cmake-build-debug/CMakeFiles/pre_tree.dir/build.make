# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.5

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


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

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/cod/workspace/ucas_network_2020/prj09

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/cod/workspace/ucas_network_2020/prj09/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/pre_tree.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/pre_tree.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/pre_tree.dir/flags.make

CMakeFiles/pre_tree.dir/test.c.o: CMakeFiles/pre_tree.dir/flags.make
CMakeFiles/pre_tree.dir/test.c.o: ../test.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/cod/workspace/ucas_network_2020/prj09/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/pre_tree.dir/test.c.o"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/pre_tree.dir/test.c.o   -c /home/cod/workspace/ucas_network_2020/prj09/test.c

CMakeFiles/pre_tree.dir/test.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/pre_tree.dir/test.c.i"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/cod/workspace/ucas_network_2020/prj09/test.c > CMakeFiles/pre_tree.dir/test.c.i

CMakeFiles/pre_tree.dir/test.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/pre_tree.dir/test.c.s"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/cod/workspace/ucas_network_2020/prj09/test.c -o CMakeFiles/pre_tree.dir/test.c.s

CMakeFiles/pre_tree.dir/test.c.o.requires:

.PHONY : CMakeFiles/pre_tree.dir/test.c.o.requires

CMakeFiles/pre_tree.dir/test.c.o.provides: CMakeFiles/pre_tree.dir/test.c.o.requires
	$(MAKE) -f CMakeFiles/pre_tree.dir/build.make CMakeFiles/pre_tree.dir/test.c.o.provides.build
.PHONY : CMakeFiles/pre_tree.dir/test.c.o.provides

CMakeFiles/pre_tree.dir/test.c.o.provides.build: CMakeFiles/pre_tree.dir/test.c.o


CMakeFiles/pre_tree.dir/base.c.o: CMakeFiles/pre_tree.dir/flags.make
CMakeFiles/pre_tree.dir/base.c.o: ../base.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/cod/workspace/ucas_network_2020/prj09/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/pre_tree.dir/base.c.o"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/pre_tree.dir/base.c.o   -c /home/cod/workspace/ucas_network_2020/prj09/base.c

CMakeFiles/pre_tree.dir/base.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/pre_tree.dir/base.c.i"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/cod/workspace/ucas_network_2020/prj09/base.c > CMakeFiles/pre_tree.dir/base.c.i

CMakeFiles/pre_tree.dir/base.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/pre_tree.dir/base.c.s"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/cod/workspace/ucas_network_2020/prj09/base.c -o CMakeFiles/pre_tree.dir/base.c.s

CMakeFiles/pre_tree.dir/base.c.o.requires:

.PHONY : CMakeFiles/pre_tree.dir/base.c.o.requires

CMakeFiles/pre_tree.dir/base.c.o.provides: CMakeFiles/pre_tree.dir/base.c.o.requires
	$(MAKE) -f CMakeFiles/pre_tree.dir/build.make CMakeFiles/pre_tree.dir/base.c.o.provides.build
.PHONY : CMakeFiles/pre_tree.dir/base.c.o.provides

CMakeFiles/pre_tree.dir/base.c.o.provides.build: CMakeFiles/pre_tree.dir/base.c.o


CMakeFiles/pre_tree.dir/poptrie.c.o: CMakeFiles/pre_tree.dir/flags.make
CMakeFiles/pre_tree.dir/poptrie.c.o: ../poptrie.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/cod/workspace/ucas_network_2020/prj09/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/pre_tree.dir/poptrie.c.o"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/pre_tree.dir/poptrie.c.o   -c /home/cod/workspace/ucas_network_2020/prj09/poptrie.c

CMakeFiles/pre_tree.dir/poptrie.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/pre_tree.dir/poptrie.c.i"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/cod/workspace/ucas_network_2020/prj09/poptrie.c > CMakeFiles/pre_tree.dir/poptrie.c.i

CMakeFiles/pre_tree.dir/poptrie.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/pre_tree.dir/poptrie.c.s"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/cod/workspace/ucas_network_2020/prj09/poptrie.c -o CMakeFiles/pre_tree.dir/poptrie.c.s

CMakeFiles/pre_tree.dir/poptrie.c.o.requires:

.PHONY : CMakeFiles/pre_tree.dir/poptrie.c.o.requires

CMakeFiles/pre_tree.dir/poptrie.c.o.provides: CMakeFiles/pre_tree.dir/poptrie.c.o.requires
	$(MAKE) -f CMakeFiles/pre_tree.dir/build.make CMakeFiles/pre_tree.dir/poptrie.c.o.provides.build
.PHONY : CMakeFiles/pre_tree.dir/poptrie.c.o.provides

CMakeFiles/pre_tree.dir/poptrie.c.o.provides.build: CMakeFiles/pre_tree.dir/poptrie.c.o


# Object files for target pre_tree
pre_tree_OBJECTS = \
"CMakeFiles/pre_tree.dir/test.c.o" \
"CMakeFiles/pre_tree.dir/base.c.o" \
"CMakeFiles/pre_tree.dir/poptrie.c.o"

# External object files for target pre_tree
pre_tree_EXTERNAL_OBJECTS =

pre_tree: CMakeFiles/pre_tree.dir/test.c.o
pre_tree: CMakeFiles/pre_tree.dir/base.c.o
pre_tree: CMakeFiles/pre_tree.dir/poptrie.c.o
pre_tree: CMakeFiles/pre_tree.dir/build.make
pre_tree: CMakeFiles/pre_tree.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/cod/workspace/ucas_network_2020/prj09/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking C executable pre_tree"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/pre_tree.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/pre_tree.dir/build: pre_tree

.PHONY : CMakeFiles/pre_tree.dir/build

CMakeFiles/pre_tree.dir/requires: CMakeFiles/pre_tree.dir/test.c.o.requires
CMakeFiles/pre_tree.dir/requires: CMakeFiles/pre_tree.dir/base.c.o.requires
CMakeFiles/pre_tree.dir/requires: CMakeFiles/pre_tree.dir/poptrie.c.o.requires

.PHONY : CMakeFiles/pre_tree.dir/requires

CMakeFiles/pre_tree.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/pre_tree.dir/cmake_clean.cmake
.PHONY : CMakeFiles/pre_tree.dir/clean

CMakeFiles/pre_tree.dir/depend:
	cd /home/cod/workspace/ucas_network_2020/prj09/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/cod/workspace/ucas_network_2020/prj09 /home/cod/workspace/ucas_network_2020/prj09 /home/cod/workspace/ucas_network_2020/prj09/cmake-build-debug /home/cod/workspace/ucas_network_2020/prj09/cmake-build-debug /home/cod/workspace/ucas_network_2020/prj09/cmake-build-debug/CMakeFiles/pre_tree.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/pre_tree.dir/depend
