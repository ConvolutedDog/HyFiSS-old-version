#!/bin/bash

SEARCH_ROOT="/home/yangjianchao/Github/memory_model/apps/OursTracesCollection"

CMD="./process_sass_dir --dir"

find "$SEARCH_ROOT" -type d -name "sass_traces" | while read dir; do
    echo "Processing directory: $dir"
    $CMD "$dir"
done


# ../simulator-apps/OursTracesCollection/cublas_GemmEx_HF_CC/128x128x128/sass_traces
# ../simulator-apps/OursTracesCollection/cublas_GemmEx_HF_CC/256x256x256/sass_traces
# ../simulator-apps/OursTracesCollection/cublas_GemmEx_HF_CC/512x512x512/sass_traces
# ../simulator-apps/OursTracesCollection/cublas_GemmEx_HF_CC/1024x1024x1024/sass_traces
# ../simulator-apps/OursTracesCollection/cublas_GemmEx_HF_CC/2048x2048x2048/sass_traces
# ../simulator-apps/OursTracesCollection/cublas_GemmEx_HF_CC/2048x2048x2048xbak/sass_traces
# ../simulator-apps/OursTracesCollection/cublas_GemmEx_HF_TC/128x128x128/sass_traces
# ../simulator-apps/OursTracesCollection/cublas_GemmEx_HF_TC/256x256x256/sass_traces
# ../simulator-apps/OursTracesCollection/cublas_GemmEx_HF_TC/512x512x512/sass_traces
# ../simulator-apps/OursTracesCollection/cusparse_spmm_csr_HF_CC/512x512x13107x512/sass_traces
# ../simulator-apps/OursTracesCollection/LULESH/cuda/sass_traces
# ../simulator-apps/OursTracesCollection/sputnik_spmm_csr_HF_CC/2048x2048x209716x2048/sass_traces
# ../simulator-apps/OursTracesCollection/sputnik_spmm_csr_HF_CC/4096x4096x838860x4096/sass_traces
# ../simulator-apps/OursTracesCollection/sputnik_spmm_csr_HF_CC/512x512x13107x512/sass_traces
# ../simulator-apps/OursTracesCollection/sputnik_spmm_csr_HF_CC/1024x1024x52428x1024/sass_traces
# ../simulator-apps/OursTracesCollection/DeepBench/conv_bench_inference_halfx700x161x1x1x32x20x5x0x0x2x2/sass_traces
# ../simulator-apps/OursTracesCollection/DeepBench/conv_bench_train_halfx700x161x1x1x32x20x5x0x0x2x2/sass_traces
# ../simulator-apps/OursTracesCollection/DeepBench/gemm_bench_inference_halfx1760x7000x1760x0x0/sass_traces
# ../simulator-apps/OursTracesCollection/DeepBench/gemm_bench_train_halfx1760x7000x1760x0x0/sass_traces
# ../simulator-apps/OursTracesCollection/DeepBench/rnn_bench_inference_halfx1024x1x25xlstm/sass_traces
# ../simulator-apps/OursTracesCollection/DeepBench/rnn_bench_train_halfx1024x1x25xlstm/sass_traces
# ../simulator-apps/OursTracesCollection/PENNANT/sass_traces
# ../simulator-apps/OursTracesCollection/PolyBench/2DCONV/sass_traces
# ../simulator-apps/OursTracesCollection/PolyBench/3DCONV/sass_traces
# ../simulator-apps/OursTracesCollection/PolyBench/3MM/sass_traces
# ../simulator-apps/OursTracesCollection/PolyBench/ATAX/sass_traces
# ../simulator-apps/OursTracesCollection/PolyBench/BICG/sass_traces
# ../simulator-apps/OursTracesCollection/PolyBench/GEMM/sass_traces
# ../simulator-apps/OursTracesCollection/PolyBench/GESUMMV/sass_traces
# ../simulator-apps/OursTracesCollection/PolyBench/GRAMSCHM/sass_traces
# ../simulator-apps/OursTracesCollection/PolyBench/MVT/sass_traces
# ../simulator-apps/OursTracesCollection/Rodinia/b+tree/sass_traces
# ../simulator-apps/OursTracesCollection/Rodinia/backprop/sass_traces
# ../simulator-apps/OursTracesCollection/Rodinia/bfs/sass_traces
# ../simulator-apps/OursTracesCollection/Rodinia/cfd/sass_traces
# ../simulator-apps/OursTracesCollection/Rodinia/dwt2d/sass_traces
# ../simulator-apps/OursTracesCollection/Rodinia/gaussian/sass_traces
# ../simulator-apps/OursTracesCollection/Rodinia/hotspot/sass_traces
# ../simulator-apps/OursTracesCollection/Rodinia/hotspot3D/sass_traces
# ../simulator-apps/OursTracesCollection/Rodinia/huffman/sass_traces
# ../simulator-apps/OursTracesCollection/Rodinia/lavaMD/sass_traces
# ../simulator-apps/OursTracesCollection/Rodinia/lud/sass_traces
# ../simulator-apps/OursTracesCollection/Rodinia/nn/sass_traces
# ../simulator-apps/OursTracesCollection/Rodinia/nw/sass_traces
# ../simulator-apps/OursTracesCollection/Rodinia/pathfinder/sass_traces
# ../simulator-apps/OursTracesCollection/Tango/AlexNet/sass_traces
# ../simulator-apps/OursTracesCollection/Tango/GRU/sass_traces
# ../simulator-apps/OursTracesCollection/Tango/LSTM/sass_traces
# ../simulator-apps/OursTracesCollection/Tango/ResNet/sass_traces
# ../simulator-apps/OursTracesCollection/Tango/SqueezeNet/sass_traces