import sys
from glob import glob
import subprocess
from os import path
import re
import time

gCompilerPath = './mycompiler'
gCompileArgs = '> /dev/null'
gGccArgs = '-L . -lsysy -march=armv7'

if __name__ == '__main__':
  # print(sys.argv)
  make_res = subprocess.call("make -j", shell=True)
  if make_res == 1:
    print("build compiler failed.")
    exit()

  # glob: sys.argv[1]
  if len(sys.argv)<2 :
    print("need a glob path.")
    exit()

  # print(sys.argv[1])
  # print(glob(sys.argv[1], recursive=True))
  files = glob(sys.argv[1], recursive=True)
  files.sort()

  for file in files:
    filepath_without_ext, _ = path.splitext(file)
    _, filename = path.split(file)
    print("{:10s}:{:35s}{:10s}:{}".format("Processing",filename,"full path",file))

    # Compile
    time_start = time.time()
    if subprocess.call(f"{gCompilerPath} {file} {gCompileArgs}",shell=True) == 1:
      print("compile failed.")
      continue
    time_end = time.time()
    compile_time = (time_end - time_start) # s
    print("{:10s}:{:.6f}s".format("compile tm",compile_time))

    asm_file = filepath_without_ext+".s"
    exec_file = filepath_without_ext+".o"
    stdin_file = filepath_without_ext+".in"
    stdout_file = filepath_without_ext+".out"

    # GCC Link
    link_cmd = f"gcc -o {exec_file} {gGccArgs} {asm_file}"
    time_start = time.time()
    if subprocess.call(link_cmd, shell=True) == 1:
      print("link failed.")
      continue
    time_end = time.time()
    link_time = (time_end - time_start) # s
    print("{:10s}:{:.6f}s".format("link time",link_time))

    # Exec
    exec_cmd = f"{exec_file}"
    if path.exists(stdin_file):
      stdin = open(stdin_file, 'r')
    else:
      stdin = None
    time_start = time.time()
    subp = subprocess.Popen(exec_cmd.split(), bufsize=0, stdin=stdin, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='utf-8')
    ret_code = subp.wait()
    time_end = time.time()
    run_time = (time_end - time_start) # s
    print("{:10s}:{:.6f}s".format("exec time",run_time))

    # exec_time = subp.stderr.read()
    # exec_time = re.search("[0-9]+H.*us",exec_time).group()
    subp.stderr.close()
    # print("{:10s}:{:35s}{:10s}:{:.6f}s".format("exec time",exec_time,"run time",run_time))

    # Exec output
    exec_out_list = str(subp.stdout.read()).split()
    subp.stdout.close()
    exec_out_list.append(str(ret_code))
    # print(exec_out_list)

    # Status
    status=""
    if not path.exists(stdout_file):
      print("output file not exist.")
      continue
    else:
      # Standard output
      with open(stdout_file, "r") as f:
        stdout_list = f.read().split()
        # print(stdout_list)
        if exec_out_list == stdout_list:
          status="OK!"
        else:
          status="ERROR!!!"
          print("{:10s}:{}".format("std out", stdout_list))
          print("{:10s}:{}".format("exec out", exec_out_list))
    print("{:10s}:{}".format("status",status))
    print()
    
  print("finish.")
        