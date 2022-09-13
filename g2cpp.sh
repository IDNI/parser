#!/bin/sh

G=./build-Release/g2cpp
SOURCE=$1
TARGET=$2

# backup TARGET
if [[ -e $TARGET ]] ; then
	NAME="$TARGET.bak"
	if [[ -e $NAME || -L $NAME ]] ; then
	    i=0
	    while [[ -e $NAME$i || -L $NAME$i ]] ; do
	        let i++
	    done
	    NAME="$NAME$i"
	fi
	mv "$TARGET" "$NAME"
fi

${G} $3 < $SOURCE >> $TARGET
