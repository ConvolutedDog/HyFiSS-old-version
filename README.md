

### Valgrind

```shell
valgrind --leak-check=full --show-leak-kinds=all --allow-mismatched-debuginfo=yes --read-inline-info=yes --read-var-info=yes --quiet --verbose --log-file=valgrind.log ./memory_model.x --configs ./traces/vectoradd-2/configs/ --sort 0 --log 0 --tmp 0 > tmp.txt
```