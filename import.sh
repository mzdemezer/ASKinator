#! /bin/bash

sudo cp -f /media/MS-DOS/ASK/*.C src/
sudo cp -f /media/MS-DOS/ASK/*.c src/
sudo chown mzajaczkowski:mzajaczkowski src/* 
cd src;
ls | while read x; do
	y=`echo $x | tr '[A-Z]' '[a-z]'`;
	echo "pre: $x";
	echo "post $y";
	if [ $x != $y ]; then
		mv $x $y;
	fi;
done

