# This file is part of Catherine Full Body HD Patch
# Copyright © 2020 浅倉麗子

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.0)

set(CMAKE_TOOLCHAIN_FILE "$ENV{DOLCESDK}/share/dolce.toolchain.cmake" CACHE PATH "DolceSDK toolchain file")
set(DOLCESDK_CMAKE_FILE "$ENV{DOLCESDK}/share/dolce.cmake" CACHE PATH "DolceSDK CMake functions file")
include("${DOLCESDK_CMAKE_FILE}" REQUIRED)

option(CMAKE_TLS_VERIFY "Verify TLS" ON)
include(ExternalProject REQUIRED)

project(catherinefbhd LANGUAGES C)

add_compile_options(
  -Os -std=c99
  -Wall -Wextra -Wdouble-promotion -Wshadow -Wundef
  -fsingle-precision-constant -fno-common
  -ftree-vectorize -funsafe-math-optimizations -flto
)

add_link_options(
  -nostdlib
  --entry=module_start
)

set(LOGGING "1" CACHE STRING "Enable logging")
set(FILE_LOGGING "0" CACHE STRING "Enable file logging")
set(LOG_LEVEL "INFO" CACHE STRING "Log verbosity level")

add_compile_definitions(
  SCE_DBG_LOGGING_ENABLED=${LOGGING}
  SCE_DBG_FILE_LOGGING_ENABLED=${FILE_LOGGING}
  SCE_DBG_MINIMUM_LOG_LEVEL=SCE_DBG_LOG_LEVEL_${LOG_LEVEL}
)

if(FILE_LOGGING)
  add_compile_definitions(SCE_DBG_LOG_PREFIX="[CatherineFBHD] ")
else()
  add_compile_definitions(SCE_DBG_LOG_PREFIX="\\033[0;36m[CatherineFBHD]\\033[0m ")
endif()

set(PATCH_MODE "PATCH_720" CACHE STRING "Catherine Full Body patch mode")
add_compile_definitions(PATCH_MODE=${PATCH_MODE})

# External dependencies

set(DEP_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/dep")
include_directories("${DEP_INSTALL_PREFIX}/include")
link_directories("${DEP_INSTALL_PREFIX}/lib")

ExternalProject_Add(dep-psp2dbg
  URL https://git.shotatoshounenwachigau.moe/vita/psp2dbg/snapshot/psp2dbg-master.tar.gz
  PREFIX dep/psp2dbg
  CMAKE_ARGS -D CMAKE_INSTALL_PREFIX=${DEP_INSTALL_PREFIX}
  EXCLUDE_FROM_ALL ON
)

ExternalProject_Add(dep-fnblit
  URL https://git.shotatoshounenwachigau.moe/vita/fnblit/snapshot/fnblit-master.tar.gz
  PREFIX dep/fnblit
  CMAKE_ARGS -D CMAKE_INSTALL_PREFIX=${DEP_INSTALL_PREFIX}
  SOURCE_SUBDIR fnblit
  EXCLUDE_FROM_ALL ON
)

ExternalProject_Add(dep-sfnconv
  URL https://git.shotatoshounenwachigau.moe/vita/fnblit/snapshot/fnblit-master.tar.gz
  PREFIX dep/sfnconv
  CMAKE_ARGS -D CMAKE_INSTALL_PREFIX=${DEP_INSTALL_PREFIX}
  SOURCE_SUBDIR sfnconv
  EXCLUDE_FROM_ALL ON
)

add_custom_target(dep-all
  DEPENDS dep-psp2dbg dep-fnblit dep-sfnconv
)

# Build font

set(FONT_BASE "terminus-font-4.48")
set(FONT_FILE "ter-u28b.bdf")
set(SFN_FILE "font.sfn")
set(SFN_OBJ "${SFN_FILE}.o")

file(DOWNLOAD
  "https://sourceforge.net/projects/terminus-font/files/${FONT_BASE}/${FONT_BASE}.tar.gz/download"
  "${FONT_BASE}.tar.gz"
  EXPECTED_HASH SHA256=34799c8dd5cec7db8016b4a615820dfb43b395575afbb24fc17ee19c869c94af
)

add_custom_command(OUTPUT "${SFN_FILE}"
  DEPENDS "${FONT_BASE}.tar.gz"
  COMMAND tar xzf "${FONT_BASE}.tar.gz" --strip-components=1 "${FONT_BASE}/${FONT_FILE}"
  COMMAND "${CMAKE_COMMAND}" -E env PATH="${DEP_INSTALL_PREFIX}/bin:$ENV{PATH}"
    bit2sfn
    -r "Basic Latin" "${FONT_FILE}"
    "${SFN_FILE}"
)

add_custom_command(OUTPUT "${SFN_OBJ}"
  DEPENDS "${SFN_FILE}"
  COMMAND "${CMAKE_LINKER}" --relocatable --format=binary --output="${SFN_OBJ}" "${SFN_FILE}"
)

# Build self

set(ELF "catherinefbhd.elf")
set(SELF "catherinefbhd.suprx")
set(MODULE_INFO "catherinefbhd.yml")

add_executable("${ELF}"
  catherinefbhd.c
  "${SFN_OBJ}"
)

target_link_libraries("${ELF}"
  fnblit
  psp2dbg

  taihen_stub

  SceLibKernel_stub
  SceSysmem_stub
)

dolce_create_self("${SELF}"
  "${ELF}"
  CONFIG "${MODULE_INFO}"
)
