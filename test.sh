# root="./sysyruntimelibrary-master/section1/functional_test/*.sy"

# for file in `ls $root`
# do
#     num=${file:53:2}
#     cond=`expr $num % 4`
#     if [ $cond -eq 1 ]
#     then
#         echo "当前的文件为："${file:53}
#         ./mycompiler $file
#         echo -e "  "
#     fi
# done

# # $file="./test/00_arr_defn2.sy"
make
./mycompiler ./test/test_func_call.sy

# 我要测试的文件名
# root="./sysyruntimelibrary-master/section1/functional_test/*.sy"

# for file in `ls $root`
# do
#     num=${file:53:2}
#     cond=`expr $num % 4`
#     if [ $cond -eq 1 ]
#     then
#         echo ${file:53}
#         # ./mycompiler $file
#         # echo -e "  "
#     fi
# done
