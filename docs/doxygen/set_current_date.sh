#! /bin/bash

# Doxygen filter for CSS files
# it is called with the name of the input file and it changes the file in place.
now=`date`
sed -e "s/current_date/$now/g" $1
