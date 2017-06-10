OUTRUN_SDK_PATH=/opt/outrun/sdk
OUTRUN_SDK_INCLUDE=${OUTRUN_SDK_PATH}/include
OUTRUN_SDK_LDSCRIPT=${OUTRUN_SDK_PATH}/ldscript
OUTRUN_SDK_LIB=${OUTRUN_SDK_PATH}/lib
OUTPUT_PATH=output

OUTRUN_GCC_PREFIX="m68k-elf-"

mkdir -p ${OUTPUT_PATH}

rm -vf ${OUTPUT_PATH}/main.link.in
rm -vf ${OUTPUT_PATH}/sub.link.in

echo "Compiling..."

for filename in *.c ../common/*.c; do
  oname="${OUTPUT_PATH}/$(basename "${filename%.*}").o"
  cpudef=CPU0
  if [[ $filename == *"sub"* ]]; then
    cpudef=CPU1
  fi

  echo "Compiling $filename to $oname"
  echo "${OUTRUN_SDK_INCLUDE}/${cpudef,,}"
  if [ "${filename##*.}" == "c" ]; then
    ${OUTRUN_GCC_PREFIX}gcc -c $filename -std=gnu11 -m68000 -o $oname -Os -D${cpudef} -I../common -I${OUTRUN_SDK_INCLUDE} -I${OUTRUN_SDK_INCLUDE}/${cpudef,,}
  elif [ "${filename##*.}" == "s" ]; then
    ${OUTRUN_GCC_PREFIX}as $filename -m68000 -o $oname --defsym ${cpudef}=1
  fi

  if [[ $cpudef == "CPU1" ]]; then
    echo ${oname} >> "${OUTPUT_PATH}/sub.link.in"
  else
    echo ${oname} >> "${OUTPUT_PATH}/main.link.in"
  fi
done

echo "Linking..."
echo "maincpu_rom.bin"
${OUTRUN_GCC_PREFIX}ld "/opt/m68k/gcc-6.3.0/lib/gcc/m68k-elf/6.3.0/m68000/crtbegin.o" $(cat ${OUTPUT_PATH}/main.link.in) ${OUTRUN_SDK_LIB}/outrun_sdk_cpu0.lib "/opt/m68k/gcc-6.3.0/lib/gcc/m68k-elf/6.3.0/m68000/crtend.o" -lc -lgcc -L"/opt/m68k/gcc-6.3.0/lib/gcc/m68k-elf/6.3.0/m68000" -L"/opt/m68k/gcc-6.3.0/m68k-elf/lib/m68000" --script=${OUTRUN_SDK_LDSCRIPT}/outrun_main_rom.ld -o ${OUTPUT_PATH}/maincpu_rom.bin --Map=${OUTPUT_PATH}/maincpu_rom.map

echo "maincpu_ram.bin"
${OUTRUN_GCC_PREFIX}ld "/opt/m68k/gcc-6.3.0/lib/gcc/m68k-elf/6.3.0/m68000/crtbegin.o" $(cat ${OUTPUT_PATH}/main.link.in) ${OUTRUN_SDK_LIB}/outrun_sdk_cpu0.lib "/opt/m68k/gcc-6.3.0/lib/gcc/m68k-elf/6.3.0/m68000/crtend.o" -lc -lgcc  -L"/opt/m68k/gcc-6.3.0/lib/gcc/m68k-elf/6.3.0/m68000" -L"/opt/m68k/gcc-6.3.0/m68k-elf/lib/m68000" --script=${OUTRUN_SDK_LDSCRIPT}/outrun_main_ram.ld -o ${OUTPUT_PATH}/maincpu_ram.bin --Map=${OUTPUT_PATH}/maincpu_ram.map

if [[ -e "${OUTPUT_PATH}/sub.link.in" ]]; then
  echo "subcpu_rom.bin"
  ${OUTRUN_GCC_PREFIX}ld "/opt/m68k/gcc-6.3.0/lib/gcc/m68k-elf/6.3.0/m68000/crtbegin.o" $(cat ${OUTPUT_PATH}/sub.link.in) ${OUTRUN_SDK_LIB}/outrun_sdk_cpu1.lib "/opt/m68k/gcc-6.3.0/lib/gcc/m68k-elf/6.3.0/m68000/crtend.o" -lc -lgcc -L"/opt/m68k/gcc-6.3.0/lib/gcc/m68k-elf/6.3.0/m68000" -L"/opt/m68k/gcc-6.3.0/m68k-elf/lib/m68000" --script=${OUTRUN_SDK_LDSCRIPT}/outrun_sub_rom.ld -o ${OUTPUT_PATH}/subcpu_rom.bin --Map=${OUTPUT_PATH}/subcpu_rom.map
  echo "subcpu_ram.bin"
  ${OUTRUN_GCC_PREFIX}ld "/opt/m68k/gcc-6.3.0/lib/gcc/m68k-elf/6.3.0/m68000/crtbegin.o" $(cat ${OUTPUT_PATH}/sub.link.in) ${OUTRUN_SDK_LIB}/outrun_sdk_cpu1.lib "/opt/m68k/gcc-6.3.0/lib/gcc/m68k-elf/6.3.0/m68000/crtend.o" -lc -lgcc -L"/opt/m68k/gcc-6.3.0/lib/gcc/m68k-elf/6.3.0/m68000" -L"/opt/m68k/gcc-6.3.0/m68k-elf/lib/m68000" --script=${OUTRUN_SDK_LDSCRIPT}/outrun_sub_ram.ld -o ${OUTPUT_PATH}/subcpu_ram.bin --Map=${OUTPUT_PATH}/subcpu_ram.map
fi

ls -sh ${OUTPUT_PATH}
