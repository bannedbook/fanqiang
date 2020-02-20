#!/bin/bash
for url in $(cat porn.txt) 
do 
  if curl -A "Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/79.0.3945.130 Safari/537.36" --connect-timeout 10 -m 30 --output /dev/null --silent --head --fail "$url"; then
    echo "$url" >> exist.txt
  else
    echo "$url"  >> notexist.txt
  fi 
done