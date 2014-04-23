#!/bin/bash

echo "" > /tmp/words.txt
#Global search for all tree

if [ `jpwdroot` == `pwd` ]; then
    echo "Using all languages files"
    #cd pkg
    find -name "*.c" -type f  | xargs grep "\<T[a-z0-9_]*"  --color=no  > /tmp/all_lang.txt
    #cd `jpwdroot`
    #cd pkg/language
    #make merge_to_csv
    #cd `jpwdflip`
    #cat all_language_files.csv | cut -f 1 -d , > /tmp/words.txt
else
    echo "Using local languages files"
    #Local search in current dir only
    find -name "*.c" -type f  | xargs grep "\<T[a-z0-9_]*"  --color=no  > /tmp/all_lang.txt
    for i in `find -name *.csv -type f`; do grep -o "^T[^,]*" $i >> /tmp/words.txt; done
fi

for i in `cat /tmp/words.txt `; do echo $i `grep "$i" /tmp/all_lang.txt -c` | grep -w 0 | grep -o "\<T[a-z0-9_]*"; done > /tmp/not_used.txt
echo "Results are in /tmp/not_used.txt"
gvim /tmp/not_used.txt

if [ $1 == "-r" ]; then
    echo "Removing..."
    #TODO: Add flag to remove redundant lines from the files (use sed)
else
    echo "Use -r flag to automatically remove from langauge files"
fi
