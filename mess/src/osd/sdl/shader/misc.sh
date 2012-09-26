#! /bin/sh

for i in *csmooth* ; do 
	cp -v $i $( echo $i | sed 's/csmooth/cgauss/g') 
done
