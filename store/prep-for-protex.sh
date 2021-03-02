#! /bin/bash 
#
# 

if [[ '1' -ne "${#}" ]]; then
	echo "Usage: $(basename ${0}) <tarball>"
	echo "Prepare a tar ball of OpenOCD suitable for Protex analysis" 
	echo "   <tarball> : openocd tar.bz2 file from /depot/devenv/tools/openocd"
	echo ""
	echo " For <tarball> with name FILENAME.tar.bz2, "
	echo " we are it to contain a directory"
	echo " with the name FILENAME in it. "
	echo "" 
	echo "The output file will be FILENAME.protex.tar.bz2"
	exit 1;
fi

INTARBALL="${1}"
if [ ! -f ${INTARBALL} ]; then
    echo "ERROR: ${INTARBALL} does not exists. Exiting ...";
	exit 1;
fi
INTARBALL="$(realpath ${1})"
DIRNAME="$(dirname ${INTARBALL})"
INBASENAME="$(basename ${INTARBALL})";
FILENAME="${INBASENAME%.tar.bz2}";
OUTTARBALL="${DIRNAME}/${FILENAME}.protex.tar.bz2"

MYTMPDIR="$(mktemp -d -t ${FILENAME}-XXXXXX)"; STATUS=$?
if [[ '0' -ne "${STATUS}" ]]; then
    echo "ERROR: Cannot create temporary directory. Exiting ..."
	exit 1;
fi
#trap "rm -fr ${MYTMPDIR}" SIGINT EXIT

pushd ${MYTMPDIR}

echo "Progress: Extracting ${INBASENAME} ..."
tar -jxf ${INTARBALL}

echo "Progress: Preparing for protex ..."
if [ -d ${FILENAME} ]; then
	pushd ${FILENAME}
fi

find . -name '.deps' -exec rm -fr {} \;
find . -name '.git*' -exec rm -fr {} \;
find . -name '.p4*' -exec rm -fr {} \;
find . -name 'Makefile.in' -exec rm -f {} \;
find . -name '.indent.pro' -exec rm -f {} \;
find . -name '.project' -exec rm -f {} \;
find . -name '.travis.yml' -exec rm -f {} \;
rm -fr contrib doc testing store 
for search in  'BUGS' 'COPYING' 'ChangeLog' 'Doxyfile.in' \
      HACKING  NEWTAPS  TODO \
      config_subdir.m4 guess-rev.sh uncrustify.cfg \
	  INSTALL aclocal.m4  \
	  compile config.guess config.h.in config.sub \
	  configure depcomp install-sh ltmain.sh mdate-sh \
	  missing texinfo.tex stamp-h1; do
	find . -name $search -exec rm -f {} \;
done
for search in 'AUTHORS'  'NEWS' 'README' autom4te.cache; do
	find . -name $search'*'  -exec rm -fr {} \;
done
rm -fr src/jtag/drivers/libjaylink/build-aux/


echo "Progress: tarball to   $(basename ${OUTTARBALL}) ..."
tar -jcf ${OUTTARBALL} *
echo "Done."