#!/bin/bash

echo "uint16_t addresses_bank$1[] = {"
for i in $(cat $2 | grep "^.....[ABCDEF]" | cut -c 6-9); do
	echo -n "0x$i, "
done
echo
echo "};"

echo "char *labels_bank$1[] = {"
for i in $(cat $2 | grep "^.....[ABCDEF]" | cut -c 12-); do
	echo -n "\"$i\", "
done
echo
echo "};"
