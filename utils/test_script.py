import sys
from glob import glob
import subprocess
from os import path
import re

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
    print("{:8s}:{:35s}{:8s}:{}".format("process",filename,"fullpath",file))

    if subprocess.call(f"./mycompiler {file} > /dev/null",shell=True) == 1:
      print("compile failed.")
      continue

    asm_file = filepath_without_ext+".s"
    exec_file = filepath_without_ext+".o"
    stdin_file = filepath_without_ext+".in"
    stdout_file = filepath_without_ext+".out"

    link_cmd = f"gcc -o {exec_file} -L . -lsysy -march=armv7 {asm_file}"
    if subprocess.call(link_cmd, shell=True) == 1:
      print("link failed.")
      continue

    exec_cmd = f"{exec_file}"
    if path.exists(stdin_file):
      stdin = open(stdin_file, 'r')
    else:
      stdin = None
    subp = subprocess.Popen(exec_cmd.split(), bufsize=0, stdin=stdin, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='utf-8')
    ret_code = subp.wait()

    exec_time = subp.stderr.read().strip()
    exec_time = exec_time[exec_time.find(' ')+1:]
    subp.stderr.close()

    exec_out_list = str(subp.stdout.read()).split()
    subp.stdout.close()
    exec_out_list.append(str(ret_code))
    # print(exec_out_list)

    status=""
    if not path.exists(stdout_file):
      print("output file not exist.")
      continue
    else:
      with open(stdout_file, "r") as f:
        stdout_list = f.read().split()
        # print(stdout_list)
        if exec_out_list == stdout_list:
          status="OK!"
        else:
          status="ERROR!!!"
    print("{:8s}:{:35s}{:8s}:{}".format("exectime",exec_time,"status",status))
    
  print("finish.")
        