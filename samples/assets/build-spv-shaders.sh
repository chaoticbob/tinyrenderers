#!/bin/sh

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ "$#" -lt 1 ]; then
  echo "Usage: build-spv-shaders.sh <glslang path>"
  exit
fi

glslang=$1
if [ ! $(which $glslang) ];  then
  echo "glslangValidator not found at $glslang"
  exit
fi

echo "Using $glslang"

function has_file_changed()
{
  filepath=$1
  filename=$(basename $filepath)

  dirpath=$script_dir/build_files
  mkdir -p $dirpath
  md5_file=$dirpath/$filename.md5

  hash_value=$(md5sum $filepath)
  modified=$(stat -c %y $filepath)
  key="$hash_value $modified"

  if [ ! -f $md5_file ]; then
    echo "$key" > $md5_file
    echo 1
    return
  fi

  if ! grep -Fxq "$key" $md5_file 
  then
    echo "$key" > $md5_file
    echo 1
    return
  fi

  echo 0
  return
}

function compile_compute() {
  filepath=$1
  changed=$(has_file_changed $filepath)
  if [ $changed -eq 0 ]; then
    echo "Skipping $filepath no changes detected"
    return 
  fi

  entry=$2
  filename=$(basename $filepath .hlsl)

  echo ""
  echo "Compiling $filepath"

  cmd="$glslang -D -V -S comp -e $entry --hlsl-iomap --auto-map-bindings --hlsl-offsets -o $filename.cs.spv $filepath"
  echo $cmd
  $cmd

  echo ""
}

function compile_vs_ps() {
  filepath=$1
  changed=$(has_file_changed $filepath)
  if [ $changed -eq 0 ]; then
    echo "Skipping $filepath no changes detected"
    return 
  fi
  
  vs_entry=$2
  ps_entry=$3
  filename=$(basename $filepath .hlsl)

  echo ""
  echo "Compiling $filepath"

  cmd="$glslang -D -V -S vert -e $vs_entry --hlsl-iomap --auto-map-bindings --hlsl-offsets -o $filename.vs.spv $filepath"
  echo $cmd
  $cmd

  cmd="$glslang -D -V -S frag -e $ps_entry --hlsl-iomap --auto-map-bindings --hlsl-offsets -o $filename.ps.spv $filepath"
  echo $cmd
  $cmd

  echo ""
}

echo ""

compile_compute append_consume.hlsl main
compile_compute byte_address_buffer.hlsl main
compile_compute simple_compute.hlsl main
compile_compute structured_buffer.hlsl main
compile_vs_ps color.hlsl VSMain PSMain
compile_vs_ps constant_buffer.hlsl VSMain PSMain
compile_vs_ps opaque_args.hlsl VSMain PSMain
compile_vs_ps passing_arrays.hlsl VSMain PSMain
compile_vs_ps texture.hlsl VSMain PSMain
compile_vs_ps uniformbuffer.hlsl VSMain PSMain

echo ""