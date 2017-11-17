#!/bin/sh

# echo on
set -x

# Create temporary build dir
build_dir=tmp_build
mkdir -p $build_dir

# @fn spirv_opt
#
#
function process_spv()
{
  spv_file=$1

  spirv-opt \
  --inline-entry-points-exhaustive \
  --convert-local-access-chains \
  --eliminate-local-single-block \
  --eliminate-local-single-store \
  --eliminate-insert-extract \
  --eliminate-dead-code-aggressive \
  --eliminate-dead-branches \
  --merge-blocks \
  --eliminate-local-single-block \
  --eliminate-local-single-store \
  --eliminate-local-multi-store \
  --eliminate-insert-extract \
  --eliminate-dead-code-aggressive \
  --eliminate-common-uniform \
  -o $build_dir/$spv_file.tmp_1 $spv_file

  spirv-opt -Os -o $spv_file $build_dir/$spv_file.tmp_1
}

# @fn disassemble_spv
#
#
function disassemble_spv()
{
  spv_file=$1
  asm_file=$2
  spirv-dis --raw-id $spv_file > $asm_file
}

# structs_01
file=structs_01
glslangValidator -V -D -Os -S vert -e vsmain -o $file.vs.spv $file.hlsl
glslangValidator -V -D -Os -S frag -e psmain -o $file.ps.spv $file.hlsl
#process_spv $file.vs.spv
#process_spv $file.ps.spv
spirv-dis --raw-id $file.vs.spv > $file.vs.spvasm
spirv-dis --raw-id $file.ps.spv > $file.ps.spvasm
disassemble_spv $file.vs.spv $file.vs.spvasm
disassemble_spv $file.ps.spv $file.ps.spvasm

# structs_02
file=structs_02
glslangValidator -V -D -Os -S vert -e vsmain -o $file.vs.spv $file.hlsl
glslangValidator -V -D -Os -S frag -e psmain -o $file.ps.spv $file.hlsl
#process_spv $file.vs.spv
#process_spv $file.ps.spv
spirv-dis --raw-id $file.vs.spv > $file.vs.spvasm
spirv-dis --raw-id $file.ps.spv > $file.ps.spvasm
disassemble_spv $file.vs.spv $file.vs.spvasm
disassemble_spv $file.ps.spv $file.ps.spvasm

# structs_03
file=structs_03
glslangValidator -V -D -Os -S vert -e vsmain -o $file.vs.spv $file.hlsl
glslangValidator -V -D -Os -S frag -e psmain -o $file.ps.spv $file.hlsl
#process_spv $file.vs.spv
#process_spv $file.ps.spv
spirv-dis --raw-id $file.vs.spv > $file.vs.spvasm
spirv-dis --raw-id $file.ps.spv > $file.ps.spvasm
disassemble_spv $file.vs.spv $file.vs.spvasm
disassemble_spv $file.ps.spv $file.ps.spvasm


# structs_04
file=structs_04
glslangValidator -V -D -Os -S vert -e vsmain -o $file.vs.spv $file.hlsl
glslangValidator -V -D -Os -S frag -e psmain -o $file.ps.spv $file.hlsl
#process_spv $file.vs.spv
#process_spv $file.ps.spv
spirv-dis --raw-id $file.vs.spv > $file.vs.spvasm
spirv-dis --raw-id $file.ps.spv > $file.ps.spvasm
disassemble_spv $file.vs.spv $file.vs.spvasm
disassemble_spv $file.ps.spv $file.ps.spvasm
