#! /bin/bash

cd src;
ls | grep .c | while read x; do
	y=`echo $x | tr '[a-z]' '[A-Z]'`;
	sudo rm "/media/MS-DOS/ASK/$y";
	sudo cp -f $x /media/MS-DOS/ASK/$y;
done

