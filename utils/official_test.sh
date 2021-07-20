root="official_test/functional_test/*.sy"
make -j
rm -rf "./official_test/functional_test/*.s"
rm -rf "./official_test/functional_test/*.o"
for file in `ls $root`
do
    # file_name=${file:51}
    echo "当前的文件为："${file:30}
    ./mycompiler $file > /dev/null
    # echo ${file_name%.*}".s"
    gcc-7 -o ${file%.*}".o" -L . -lsysy -march=armv7 ${file%.*}".s" 
    echo "我们的输出："
    # echo ${file%.*}".in"
    if [ -f ${file%.*}".in" ]; then
        # echo ${file%.*}".in"
        ${file%.*}".o" < ${file%.*}".in"
        echo $?
    else
        ${file%.*}".o"
        echo $?
    fi
    echo "标准输出为："
    cat ${file%.*}".out"
    echo -e "  "
done