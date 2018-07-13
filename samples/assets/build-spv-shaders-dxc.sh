#!/bin/sh

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
compile_flags="-O3 -fvk-use-dx-layout"

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
  stage=$2

  dirpath=$script_dir/tmp_build_files
  mkdir -p $dirpath
  md5_file=$dirpath/$filename.$stage.md5

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

function compile_gs() {
  filepath=$1
  entry=$2
  filename=$(basename $filepath .hlsl)
  output_filename=$filename.gs.spv

  build=false
  if [ ! -f $output_filename ]; then
    build=true
  fi

  if [ "$build" = false ]; then
    changed=$(has_file_changed $filepath gs)
    if [ $changed -eq 0 ]; then
      echo "Skipping $filepath no changes detected"
      return
    fi
  fi


  echo ""
  echo "Compiling $filepath"

  cmd="$dxc_exe -spirv $compile_flags -T gs_6_0 -E $entry -Fo $output_filename $filepath"
  echo $cmd
  $cmd

  echo ""
}

function compile_cs() {
  filepath=$1
  entry=$2
  filename=$(basename $filepath .hlsl)
  output_filename=$filename.cs.spv

  build=false
  if [ ! -f $output_filename ]; then
    build=true
  fi

  if [ "$build" = false ]; then
    changed=$(has_file_changed $filepath cs)
    if [ $changed -eq 0 ]; then
      echo "Skipping $filepath no changes detected"
      return
    fi
  fi


  echo ""
  echo "Compiling $filepath"

  cmd="$dxc_exe -spirv $compile_flags -T cs_6_0 -E $entry -Fo $output_filename $filepath"
  echo $cmd
  $cmd

  echo ""
}

function compile_hs_ds() {
  filepath=$1
  vs_entry=$2
  ps_entry=$3
  filename=$(basename $filepath .hlsl)
  output_hs_filename=$filename.hs.spv
  output_ds_filename=$filename.ds.spv

  build_hs=false
  if [ ! -f $output_hs_filename ]; then
    build_hs=true
  fi

  build_ds=false
  if [ ! -f $output_ds_filename ]; then
    build_ds=true
  fi

  changed=$(has_file_changed $filepath vs_ds)
  if [ $changed -eq 1 ]; then
    build_hs=true
    build_ds=true
  fi


  if [ "$build_hs" = false ] && [ "$build_ds" = false ]; then
    echo "Skipping $filepath no changes detected"
    return
  fi

  echo ""
  echo "Compiling $filepath"

  if [ "$build_hs" = true ]; then
    cmd="$dxc_exe -spirv $compile_flags -T hs_6_0 -E $vs_entry -Fo $output_hs_filename $filepath"
    echo $cmd
    $cmd
  fi

  if [ "$build_ds" = true ]; then
    cmd="$dxc_exe -spirv $compile_flags -T ds_6_0 -E $ps_entry -Fo $output_ds_filename $filepath"
    echo $cmd
    $cmd
  fi

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

  changed=$(has_file_changed $filepath vs_ps)
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
    cmd="$dxc_exe -spirv $compile_flags -T vs_6_0 -E $vs_entry -Fo $output_vs_filename $filepath"
    echo $cmd
    $cmd
  fi

  if [ "$build_ps" = true ]; then
    cmd="$dxc_exe -spirv $compile_flags -T ps_6_0 -E $ps_entry -Fo $output_ps_filename $filepath"
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

compile_vs_ps triangle_wireframe.hlsl VSMain PSMain
compile_gs triangle_wireframe.hlsl GSMain

compile_vs_ps simple_tess_color.hlsl VSMain PSMain
compile_vs_ps simple_tess_isoline.hlsl VSMain PSMain
compile_hs_ds simple_tess_isoline.hlsl HSMain DSMain

compile_cs debugging.hlsl main

echo ""
