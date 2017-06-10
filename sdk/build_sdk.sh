#!/bin/bash

OUTRUN_GCC_PREFIX="m68k-elf-"
OUTRUN_SDK_INCLUDE="include"

echo "Building the Sega Out Run SDK..."

for j in 0 1; do
  inputdir="cpu$j"
  outputdir="obj/cpu$j"
  cpudef="CPU$j"

  rm -fv ${outputdir}/lib.in
  
  for filename in src/${inputdir}/*.c src/${inputdir}/*.s; do
    oname="${outputdir}/$(basename "${filename%.*}").o"

    if [ "$1" == "clean" ]; then
	    rm -fv $oname
    else
      echo "Compiling $filename to $oname"
      mkdir -p $outputdir

      if [ "${filename##*.}" == "c" ]; then
        ${OUTRUN_GCC_PREFIX}gcc -c $filename -std=gnu11 -m68000 -o $oname -Os -D${cpudef} -I${OUTRUN_SDK_INCLUDE} -I${OUTRUN_SDK_INCLUDE}/cpu$j
      elif [ "${filename##*.}" == "cpp" ]; then
        ${OUTRUN_GCC_PREFIX}g++ -c $filename --no-rtti -m68000 -o $oname -Os -D${cpudef} --no-exceptions -I${OUTRUN_SDK_INCLUDE} -I${OUTRUN_SDK_INCLUDE}/cp$j
      elif [ "${filename##*.}" == "s" ]; then
        ${OUTRUN_GCC_PREFIX}as $filename -m68000 -o $oname --defsym ${cpudef}=1
      fi

      echo "$oname" >> ${outputdir}/lib.in
    fi
  done

  ls -sh "obj/cpu${j}/"
  
  echo "remove old libraries"
  libname="outrun_sdk_cpu${j}.lib"
  rm -fv lib/${libname}

  if [ "$1" != "clean" ]; then
    if [ -e "${outputdir}/lib.in" ]; then
      echo "Creating library..."
      mkdir -p lib
      ${OUTRUN_GCC_PREFIX}ar --target=elf32-m68k lib/${libname} -v -c -r $(cat ${outputdir}/lib.in)
      # cat ${outputdir}/lib.in | while read ofile; do
      #  ${OUTRUN_GCC_PREFIX}ar --target=elf32-m68k lib/${libname} -v -c -r ${ofile}
      # done
    fi
  fi
done

echo "Done."
