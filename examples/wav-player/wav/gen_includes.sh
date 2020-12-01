#!/bin/bash
mkdir -p include

for file in *.wav; do
	out="include/$(echo $file | cut -f 1 -d '.').h"
	printf "const " > $out
	xxd -i $file   >> $out
done

