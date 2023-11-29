

### Valgrind

The following describes how to compile and use `Valgrind` to detect potential problems with the simulator.

`make`: This is a ***make*** command that is used to compile the program for usage of `Valgrind`. `USE_BOOST=1` and `DEBUG=1` are variables passed to the ***Makefile*** to control the compilation process. The `MPI_HOME` and `BOOST_HOME` variables are used to specify the installation path for ***MPI*** and ***Boost***, and if they are not in the default lookup ***PATH***, you need to specify them manually.

`valgrind`: This is a ***valgrind*** command to run the program and check for memory leaks. `./memory_model.x` is the program to run.

```shell
make DEBUG=1

valgrind --leak-check=full --show-leak-kinds=all --allow-mismatched-debuginfo=yes --read-inline-info=yes --read-var-info=yes --quiet --verbose -v --log-file=valgrind.log ./memory_model.x --configs ./traces/vectoradd-2/configs/ --sort 0 --log 0 --tmp 0 > tmp.txt
```

### perf

Both of the following commands are running an MPI program, which requires 6 processes to run. The difference between these two commands is how they collect the program's performance data.

`perf stat`: This command uses perf stat to collect performance statistics such as CPU usage, number of context switches, etc. These statistics will be displayed on the terminal after the end of the program.

`perf record`: This command uses perf record to collect performance events such as CPU task-clock, branch prediction errors, etc. These events will be logged in a file called perf.data, and you can view them using the perf report command.


```shell
perf stat mpirun -np 6 ./memory_model.x --configs ./traces/vectoradd-3/configs/ --sort 0 --log 0 --tmp 0 > tmp.txt
perf record mpirun -np 6 ./memory_model.x --configs ./traces/vectoradd-3/configs/ --sort 0 --log 0 --tmp 0 > tmp.txt
```