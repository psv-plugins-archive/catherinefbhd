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
  COMMAND bit2sfn
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
set(VELF_CFG "exports.yml")

add_executable("${ELF}"
  main.c
  "${SFN_OBJ}"
)

if(LOG_PRINTF)
  target_compile_definitions("${ELF}" PRIVATE LOG_PRINTF)
endif()

if(PATCH_MODE)
  target_compile_definitions("${ELF}" PRIVATE PATCH_MODE=${PATCH_MODE})
else()
  target_compile_definitions("${ELF}" PRIVATE PATCH_MODE=0)
endif()

target_link_libraries("${ELF}"
  fnblit

  taihen_stub

  SceLibKernel_stub
  SceSysmem_stub
)

dolce_create_self("${SELF}"
  "${ELF}"
  CONFIG "${VELF_CFG}"
)
