echo "--Compiler compiled--"
./build/igel.exe ./test.ig
exit_code=$?
echo "Exit code:" $exit_code
if [ $exit_code -ne 0 ]; then
    exit $exit_code
fi
echo "--Compiled Start of Programm--"
./out
echo "Exit code:" $?