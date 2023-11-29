

### Valgrind

The following paragraph describes how to compile and use `Valgrind` to detect potential problems with the simulator.

`make`: This is a ***make*** command that is used to compile the program for usage of `Valgrind`. `USE_BOOST=1` and `DEBUG=1` are variables passed to the ***Makefile*** to control the compilation process. The `MPI_HOME` and `BOOST_HOME` variables are used to specify the installation path for ***MPI*** and ***Boost***, and if they are not in the default lookup ***PATH***, you need to specify them manually.

`valgrind`: This is a ***valgrind*** command to run the program and check for memory leaks. `./memory_model.x` is the program to run.

```shell
make DEBUG=1

valgrind --leak-check=full --show-leak-kinds=all --allow-mismatched-debuginfo=yes --read-inline-info=yes --read-var-info=yes --quiet --verbose --log-file=valgrind.log ./memory_model.x --configs ./traces/vectoradd-2/configs/ --sort 0 --log 0 --tmp 0 > tmp.txt
```