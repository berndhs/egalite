#!/bin/bash

NAME=egalite
VERSION=0.1.0
PACKAGEDIR=${HOME}/packaging/dchat

makearchive.sh ${NAME}-${VERSION} master
cp ${NAME}-${VERSION}.tar.gz ${PACKAGEDIR}
ls -l ${PACKAGEDIR}/${NAME}-${VERSION}.tar.gz

if [ x$1 == "xmake" ]
then
  cd ${PACKAGEDIR}
  make
fi
