#[[#
#
# Copyright (c) 2020 RogueLogix
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
#]]
#

add_lib(Networking)
add_lib_dependency(Threading)
add_external_lib_dependency(Boost::boost)

include(CheckIncludeFile)

check_include_file("infiniband/verbs.h" HAVE_IB_VERBS)
if (HAVE_IB_VERBS)
    target_compile_definitions(RogueLib_Networking PUBLIC ROGUELIB_NETWORKING_ENABLE_INFINIBAND)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib)
    add_external_lib_dependency(Infinity)
endif ()


