#! /bin/bash
#
#############################################

QUARTUS_ROOT_BASE=/p/psg/swip/releases/quartuskit/; # 21.1/151/linux64/quartus/

if [[ '3' -lt "${#}" ]]; then
  echo "Usage: $(basename ${)}) <outzipfile> <linux> <windows>"
  echo "Create a zip file containing OpenOCD executables suitable for BDBA analysis"
  echo "  <outzipfile>: Output zip file name."
  echo "  <linux>,<windows>: <release>/<build>, e.g. 21.1/125"
  echo "                     use: NONE if you don't want to build for the OS"
  exit 1
fi

## Read options
OUTZIPFILE="${1}"
LINUX_RELEASE="${2}"
WINDOWS_RELEASE="${3}"


OUTDIR="$(dirname ${OUTZIPFILE})";
OUTDIR="$(cd ${OUTDIR} && pwd)"
OUTZIPFILE="${OUTDIR}/$(basename ${OUTZIPFILE})"


function zip_release() {
local  ZIPFILE="${1}"
local  RELEASE="${2}"
local  OSTYPE="${3}"


local  QUARTUS_DIR="${QUARTUS_ROOT_BASE}/${RELEASE}/windows64/quartus"
local  BINARIES=( "bin64/openocd.exe" "bin64/jtagquery.exe" "bin64/libaji_client.dll" "bin64/oocd" )
if [ 'olinux64' == "o${OSTYPE}" ]; then
  QUARTUS_DIR="${QUARTUS_ROOT_BASE}/${RELEASE}/linux64/quartus"
  BINARIES=( "linux64/openocd" "linux64/jtagquery" )
  pushd ${QUARTUS_DIR}/ >/dev/null 2>/dev/null
  BINARIES+=( linux64/libaji_client* )
  BINARIES+=( lib64/libaji_client*   )
  BINARIES+=( linux64/oocd )
  popd  >/dev/null 2>/dev/null
fi
  
  if [ ! -d "${QUARTUS_DIR}" ]; then
    echo "ERROR: Directory ${QUARTUS_DIR} does not exists"
    exit 1;
  fi

  ## Prep work dir
  MYTMPDIR="$(basename ${ZIPFILE})"
  MYTMPDIR="$(mktemp -d -t ${MYTMPDIR}-${OSTYPE}-XXXXXX)"; STATUS=$?
  if [[ '0' -ne "${STATUS}" ]]; then
    echo "ERROR: Cannot create temporary directory. Exiting ..."
	exit 1;
  fi

  pushd ${MYTMPDIR}  >/dev/null 2>/dev/null

  for file in ${BINARIES[@]}; do
echo $file
    mkdir -p $(dirname ${file}) 2>/dev/null
    cp --recursive --dereference ${QUARTUS_DIR}/${file} ${file} 
  done
  zip -r ${ZIPFILE} .
  popd  >/dev/null 2>/dev/null # MYTMPDIR

} #end function zip_release


if [ 'oNONE' != "o${LINUX_RELEASE}" ]; then
   zip_release "${OUTZIPFILE}" "${LINUX_RELEASE}" "linux64"
fi

if [ 'oNONE' != "o${WINDOWS_RELEASE}" ]; then
   zip_release "${OUTZIPFILE}" "${WINDOWS_RELEASE}" "win64"
fi

