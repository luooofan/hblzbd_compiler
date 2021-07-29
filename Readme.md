# hblzbd_compiler

## Build

```shell
make -j
```

## Run

**Usage: ./compiler [-S] [-l log_file] [-o output_file] [-O level] input_file**

```shell
./compiler -S -o testcase.s testcase.sy
```

## Test

**Usage: test_script.py [-h] [-r] [-v] [-L LINKED_LIBRARY_PATH] test_path**

```shell
python3 ./utils/test_script.py "./official_test/functional_test/*.sy" -r
python3 ./utils/test_script.py "./official_test/performance_test/*.sy" -r -v 
```