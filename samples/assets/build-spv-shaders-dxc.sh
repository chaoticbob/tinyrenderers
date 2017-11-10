#!/bin/sh

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ "$#" -lt 1 ]; then
  echo "Usage: build-spv-shaders.sh <dxc path>"
  exit
fi

dxc_exe=$1
if [ ! $(which $dxc_exe) ];  then
  echo "glslangValidator not found at $dxc_exe"
  exit
fi

echo "Using $dxc_exe"

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

function compile_cs() {
  filepath=$1
  entry=$2
  filename=$(basename $filepath .hlsl)
  ouptput_filename=$filename.cs.spv

  build=false
  if [ ! -f $ouptput_filename ]; then
    build=true
  fi

  if [ "$build" = false ]; then
    changed=$(has_file_changed $filepath $ouptput_filename)
    if [ $changed -eq 0 ]; then
      echo "Skipping $filepath no changes detected"
      return
    fi
  fi


  echo ""
  echo "Compiling $filepath"

  cmd="$dxc_exe -spirv -T cs_6_0 -E $entry -Fo $ouptput_filename $filepath"
  echo $cmd
  $cmd

  echo ""
}

function compile_vs_ps() {
  filepath=$1
  vs_entry=$2
  ps_entry=$3
  filename=$(basename $filepath .hlsl)
  output_vs_filename=$filename.vs.spv
  output_ps_filename=$filename.ps.spv

  build_vs=false
  if [ ! -f $output_vs_filename ]; then
    build_vs=true
  fi

  build_ps=false
  if [ ! -f $output_ps_filename ]; then
    build_ps=true
  fi

  changed=$(has_file_changed $filepath)
  if [ $changed -eq 1 ]; then
    build_vs=true
    build_ps=true
  fi


  if [ "$build_vs" = false ] && [ "$build_ps" = false ]; then
    echo "Skipping $filepath no changes detected"
    return
  fi

  echo ""
  echo "Compiling $filepath"

  if [ "$build_vs" = true ]; then
    cmd="$dxc_exe -spirv -T vs_6_0 -E $vs_entry -Fo $output_vs_filename $filepath"
    echo $cmd
    $cmd
  fi

  if [ "$build_ps" = true ]; then
    cmd="$dxc_exe -spirv -T ps_6_0 -E $ps_entry -Fo $output_ps_filename $filepath"
    echo $cmd
    $cmd
  fi

  echo ""
}

echo ""

compile_cs append_consume.hlsl main
compile_cs byte_address_buffer.hlsl main
compile_cs simple_compute.hlsl main
compile_cs structured_buffer.hlsl main

compile_vs_ps simple.hlsl VSMain PSMain
compile_vs_ps color.hlsl VSMain PSMain
compile_vs_ps constant_buffer.hlsl VSMain PSMain
compile_vs_ps opaque_args.hlsl VSMain PSMain
compile_vs_ps passing_arrays.hlsl VSMain PSMain
compile_vs_ps texture.hlsl VSMain PSMain
compile_vs_ps textured_cube.hlsl VSMain PSMain
compile_vs_ps uniformbuffer.hlsl VSMain PSMain

echo ""
