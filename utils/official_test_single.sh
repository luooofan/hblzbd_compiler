make -j
file=$1
echo "当前的文件为："${file}
./mycompiler $file > /dev/null
# echo ${file_name%.*}".s"
gcc-7 -o ${file%.*}".o" -L . -lsysy -march=armv7 ${file%.*}".s" 
echo "我们的输出："
if [ -f ${file%.*}".in" ]; then
    ${file%.*}".o" < ${file%.*}".in"
    echo $?
else
    ${file%.*}".o"
    echo $?
fi
if [ -f ${file%.*}".out" ]; then
    echo "标准输出为："
    cat ${file%.*}".out"
    echo -e "  "
fi