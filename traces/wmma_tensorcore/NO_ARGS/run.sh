set -e

export CUDA_VERSION="11.0"; export CUDA_VISIBLE_DEVICES="1" ; export TRACES_FOLDER=/home/yangjianchao/Github/accel-sim-framework-dev/hw_run/traces/device-1/11.0/wmma_tensorcore/NO_ARGS/traces; CUDA_INJECTION64_PATH=/home/yangjianchao/Github/accel-sim-framework-dev/util/tracer_nvbit/tracer_tool/tracer_tool.so ; LD_PRELOAD=/home/yangjianchao/Github/accel-sim-framework-dev/util/tracer_nvbit/tracer_tool/tracer_tool.so /home/yangjianchao/Github/accel-sim-framework-dev/gpu-app-collection/src/..//bin/11.0/release/wmma_tensorcore  ; /home/yangjianchao/Github/accel-sim-framework-dev/util/tracer_nvbit/tracer_tool/traces-processing/post-traces-processing /home/yangjianchao/Github/accel-sim-framework-dev/hw_run/traces/device-1/11.0/wmma_tensorcore/NO_ARGS/traces/kernelslist ; rm -f /home/yangjianchao/Github/accel-sim-framework-dev/hw_run/traces/device-1/11.0/wmma_tensorcore/NO_ARGS/traces/*.trace ; rm -f /home/yangjianchao/Github/accel-sim-framework-dev/hw_run/traces/device-1/11.0/wmma_tensorcore/NO_ARGS/traces/kernelslist 