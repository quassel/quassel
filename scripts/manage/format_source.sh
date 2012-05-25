#!/bin/sh 

# format_source.sh dirname  - indent the whole source tree
# format_source.sh filename - indent a single file

if [ -d "$1" ]; then
#echo "Dir ${1} exists"

file_list=`find ${1} -name "*.cpp" -or -name "*.h" -type f`
for file2indent in $file_list
do 
echo "Indenting file $file2indent"
#!/bin/bash
uncrustify -f "$file2indent" -c "./format_source.cfg" -o indentoutput.tmp
mv indentoutput.tmp "$file2indent"

done
else
if [ -f "$1" ]; then
echo "Indenting one file $1"
#!/bin/bash
uncrustify -f "$1" -c "./format_source.cfg" -o indentoutput.tmp
mv indentoutput.tmp "$1"

else
echo "ERROR: As parameter given directory or file does not exist!"
echo "Syntax is: format_source.sh dirname filesuffix"
echo "Syntax is: format_source.sh filename"
echo "Example: format_source.sh temp cpp"
exit 1
fi
fi
