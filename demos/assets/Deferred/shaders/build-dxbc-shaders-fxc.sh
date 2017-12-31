#!/bin/sh

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
compile_flags=

if [ "$#" -lt 1 ]; then
  echo "Usage: build-dxbc-shaders-fxc.sh <fxc path>"
  exit
fi

sc_exe=$1
if [ ! "$(which $sc_exe)" ];  then
  echo "fxc not found at $sc_exe"
  exit
fi

echo "Using $sc_exe"

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

function compile_vs() {
  filepath=$1
  entry=$2
  filename=$(basename $filepath .hlsl)
  output_filename=$filename.vs.dxbc

  build=false
  if [ ! -f $output_filename ]; then
    build=true
  fi

  if [ "$build" = false ]; then
    changed=$(has_file_changed $filepath vs)
    if [ $changed -eq 0 ]; then
      echo "Skipping $filepath no changes detected"
      return
    fi
  fi


  echo ""
  echo "Compiling $filepath"

  cmd="cmd /C $sc_exe /nologo /T vs_5_1 /E $entry /Fo $output_filename $filepath && exit"
  echo $cmd
  cmd //C "$sc_exe /nologo /T vs_5_1 /E $entry /Fo $output_filename $filepath"

  echo ""
}


function compile_gs() {
  filepath=$1
  entry=$2
  filename=$(basename $filepath .hlsl)
  output_filename=$filename.gs.dxbc

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

  cmd="$sc_exe -spirv $compile_flags -T gs_5_1 -E $entry -Fo $output_filename $filepath"
  echo $cmd
  $cmd

  echo ""
}

function compile_ps() {
  filepath=$1
  entry=$2
  filename=$(basename $filepath .hlsl)
  output_filename=$filename.ps.dxbc

  build=false
  if [ ! -f $output_filename ]; then
    build=true
  fi

  if [ "$build" = false ]; then
    changed=$(has_file_changed $filepath ps)
    if [ $changed -eq 0 ]; then
      echo "Skipping $filepath no changes detected"
      return
    fi
  fi


  echo ""
  echo "Compiling $filepath"

  cmd="cmd /C $sc_exe /nologo /T ps_5_1 /E $entry $compile_flags /Fo $output_filename $filepath && exit"
  echo $cmd
  cmd //C "$sc_exe /nologo /T ps_5_1 /E $entry $compile_flags /Fo $output_filename $filepath"

  echo ""
}

function compile_cs() {
  filepath=$1
  entry=$2
  filename=$(basename $filepath .hlsl)
  output_filename=$filename.cs.dxbc

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

  cmd="$sc_exe -spirv $compile_flags -T cs_5_1 -E $entry -Fo $output_filename $filepath"
  echo $cmd
  $cmd

  echo ""
}

function compile_vs_ps() {
  filepath=$1
  vs_entry=$2
  ps_entry=$3
  filename=$(basename $filepath .hlsl)
  output_vs_filename=$filename.vs.dxbc
  output_ps_filename=$filename.ps.dxbc

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
    cmd="$sc_exe -spirv $compile_flags -T vs_6_0 -E $vs_entry -Fo $output_vs_filename $filepath"
    echo $cmd
    $cmd
  fi

  if [ "$build_ps" = true ]; then
    cmd="$sc_exe -spirv $compile_flags -T ps_6_0 -E $ps_entry -Fo $output_ps_filename $filepath"
    echo $cmd
    $cmd
  fi

  echo ""
}

echo ""

compile_vs surface.vs.hlsl vsmain
compile_ps hard_surface.ps.hlsl psmain
compile_ps velvet_surface.ps.hlsl psmain

echo ""
