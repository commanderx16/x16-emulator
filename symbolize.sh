echo "uint16_t addresses[] = {"
for i in $(cat $1 | grep "^.....[ABCDEF]" | cut -c 6-9); do
	echo -n "0x$i, "
done
echo
echo "};"

echo "char *labels[] = {"
for i in $(cat $1 | grep "^.....[ABCDEF]" | cut -c 12-); do
	echo -n "\"$i\", "
done
echo
echo "};"
