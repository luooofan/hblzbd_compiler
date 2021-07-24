from glob import glob
import subprocess
from os import path
import re
import os
import time
import argparse

if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='Automaticly test')
  parser.add_argument("test_path", type=str, help="the path of the test cases. can be glob style.")
  parser.add_argument("-r", "--run", help="link the asm files and run the executable files.", action="store_true")
  parser.add_argument("-v", "--verbose", help="print compile time and/or(depend on the option [-r]) exec time for every test case.", action="store_true")
  parser.add_argument("-L", "--linked_library_path", type=str, help="the path of the linked library.", default=".")
  args = parser.parse_args()

  CompilerPath = './compiler'
  CompileArgs = '-S -O1'
  GccArgs = f'-L {args.linked_library_path} -lsysy -march=armv7'
  OutputFolder = './autotest_output'

  if not path.exists(OutputFolder):
    os.system(f"mkdir {OutputFolder}")
  else:
    os.system(f"rm -rf {OutputFolder}/*")

  make_res = subprocess.call("make -j", shell=True)
  if make_res != 0:
    print("build compiler failed.")
    exit()
  print()

  files = glob(args.test_path, recursive=True)
  files.sort()
  bug_num = len(files)

  for file in files:
    filepath_noext, _ = path.splitext(file)
    _, filename = path.split(file)
    filename_noext, _=path.splitext(filename)
    asm_file = OutputFolder+"/"+filename_noext+".s"
    exec_file = OutputFolder+"/"+filename_noext+".o"
    log_file = OutputFolder+"/"+filename_noext+".log"
    stdin_file = filepath_noext+".in"
    stdout_file = filepath_noext+".out"

    print("{:10s}:{:35s}{:10s}:{}".format("Processing",filename,"full path",file))

    # Compile
    time_start = time.time()
    if subprocess.call(f"{CompilerPath} {file} -o {asm_file} {CompileArgs} -l {log_file}".split()) != 0:
      print("compile failed.")
      continue
    time_end = time.time()
    compile_time = (time_end - time_start) # s
    if args.verbose:
      print("{:10s}:{:.6f}s".format("compile tm",compile_time))

    if not args.run:
      bug_num-=1
      print("{:10s}:{}".format("status","OK!"))
    else:
      # GCC Link
      link_cmd = f"gcc -o {exec_file} {GccArgs} {asm_file}"
      time_start = time.time()
      if subprocess.call(link_cmd.split()) != 0:
        print("link failed.")
        continue
      time_end = time.time()
      link_time = (time_end - time_start) # s
      # print("{:10s}:{:.6f}s".format("link time",link_time))

      # Exec
      exec_cmd = f"{exec_file}"
      if path.exists(stdin_file):
        stdin = open(stdin_file, 'r')
      else:
        stdin = None
      subp = subprocess.Popen(exec_cmd.split(), bufsize=0, stdin=stdin, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='utf-8')
      exec_out, exec_err = subp.communicate()
      ret_code = subp.returncode
      exec_time = (re.search("[0-9]+H.*us",exec_err))
      if exec_time != None:
        exec_time=exec_time.group()
      if args.verbose:
        print("{:10s}:{}".format("exec time",exec_time))

      # Exec output
      exec_out_list = exec_out.split()
      exec_out_list.append(str(ret_code))
      # print(exec_out_list)

      # Status
      status=""
      if not path.exists(stdout_file):
        print("{:10s}:{}".format("exec out", exec_out_list))
        print("output file not exist.")
        continue
      else:
        # Standard output
        with open(stdout_file, "r") as f:
          stdout_list = f.read().split()
          # print(stdout_list)
          if exec_out_list == stdout_list:
            status="OK!"
            bug_num-=1
          else:
            status="ERROR!!!"
            print("{:10s}:{}".format("std out", stdout_list))
            print("{:10s}:{}".format("exec out", exec_out_list))
            # print("{:10s}:{}".format("exec err", exec_err))
      print("{:10s}:{}".format("status",status))
    print()
    
  print("{:10s}:{}".format("bug num",bug_num))
  print("finish.")
        