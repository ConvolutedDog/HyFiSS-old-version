#!/bin/python
# -*- coding: utf-8 -*-

'''
This script is used to combine the results of parallel simulations 
of multiple processes into one, and output the performance of the 
entire program simulation.
'''

import os
import re
import glob
import argparse

# ========================================================

SMs_num = 80
DEBUG = 0

# ========================================================
Thread_block_limit_SM = None
Thread_block_limit_registers = None
Thread_block_limit_shared_memory = None
Thread_block_limit_warps = None
Theoretical_max_active_warps_per_SM = None
Theoretical_occupancy = None

Unified_L1_cache_hit_rate = [None for _ in range(SMs_num)]
Unified_L1_cache_requests = [None for _ in range(SMs_num)]

L2_cache_hit_rate = None
L2_cache_requests = None

GMEM_read_requests = [None for _ in range(SMs_num)]
GMEM_write_requests = [None for _ in range(SMs_num)]
GMEM_total_requests = [None for _ in range(SMs_num)]
GMEM_read_transactions = [None for _ in range(SMs_num)]
GMEM_write_transactions = [None for _ in range(SMs_num)]
GMEM_total_transactions = [None for _ in range(SMs_num)]

Number_of_read_transactions_per_read_requests = [None for _ in range(SMs_num)]
Number_of_write_transactions_per_write_requests = [None for _ in range(SMs_num)]

L2_read_transactions = [None for _ in range(SMs_num)]
L2_write_transactions = [None for _ in range(SMs_num)]
L2_total_transactions = [None for _ in range(SMs_num)]

DRAM_total_transactions = None

Total_number_of_global_atomic_requests = [None for _ in range(SMs_num)]
Total_number_of_global_reduction_requests = [None for _ in range(SMs_num)]
Global_memory_atomic_and_reduction_transactions = [None for _ in range(SMs_num)]

Achieved_active_warps_per_SM = [None for _ in range(SMs_num)]
Achieved_occupancy = [None for _ in range(SMs_num)]

GPU_active_cycles = [None for _ in range(SMs_num)]
SM_active_cycles = [None for _ in range(SMs_num)]

Warp_instructions_executed = [None for _ in range(SMs_num)]
Instructions_executed_per_clock_cycle_IPC = [None for _ in range(SMs_num)]
Total_instructions_executed_per_seconds = [None for _ in range(SMs_num)]

Kernel_execution_time = [None for _ in range(SMs_num)]
Simulation_time_memory_model = [None for _ in range(SMs_num)]
Simulation_time_compute_model = [None for _ in range(SMs_num)]

# ========================================================
parser = argparse.ArgumentParser(description='Merge rank reports.')

parser.add_argument('--dir', type=str, required=True,
                    help='The directory of rank reports')
parser.add_argument('--kernel_id', type=int, required=True,
                    help='The kernel_id of reports')
args = parser.parse_args()
# ========================================================

# Suppose that the kernel number and all ranks are sequential, 
# and we know the kernel number.
kernel_id = args.kernel_id
reports_dir = args.dir
reports_dir = os.path.abspath(reports_dir)

print(reports_dir)

file_name_template = reports_dir + "/" + r"kernel-" + str(kernel_id) + "-rank-*.temp.txt"

# Get all the relevant documents.
files = glob.glob(file_name_template)

# ========================================================
# Parse all rank numbers to find out the maximum value.
rank_nums = [int(re.search('-rank-(\d+).temp.txt', file).group(1)) for file in files]
all_ranks_num = max(rank_nums) + 1

print("Processes number: ", all_ranks_num)
# ========================================================

# Process each file.
for file in files:
    print("Processing ", reports_dir + "/" + file.split("/")[-1], "...")
    with open(file, 'r') as f:
        content = f.read()
    # print(content)
    content = content.replace("-nan", "0")
    
    # Extract rank_num.
    match = re.search('-rank-(\d+).temp.txt', file)
    if match:
        rank_num = int(match.group(1))
        # print("Current rank: ", rank_num)
    # ========================================================
    # Define the regular expression for which we want to extract information.
    patterns = {
        'Thread_block_limit_SM': r"Thread_block_limit_SM = (\d+)",
        'Thread_block_limit_registers': r"Thread_block_limit_registers = (\d+)",
        'Thread_block_limit_shared_memory': r"Thread_block_limit_shared_memory = (\d+)",
        'Thread_block_limit_warps': r"Thread_block_limit_warps = (\d+)",
        'Theoretical_max_active_warps_per_SM': r"Theoretical_max_active_warps_per_SM = (\d+)",
        'Theoretical_occupancy': r"Theoretical_occupancy = (\d+)",
        
        'L2_cache_requests': r"L2_cache_requests = (\d+)",
        
        'DRAM_total_transactions': r"DRAM_total_transactions = (\d+)",
    }

    # Loop through the pattern dictionary to extract the corresponding value for each one.
    if rank_num == 0:
        extracted_values = {}
        for key, pattern in patterns.items():
            match = re.search(pattern, content)
            if match:
                extracted_values[key] = int(match.group(1))
            else:
                extracted_values[key] = None
                print("No match found for ", key)
                exit(0)

        for key, value in extracted_values.items():
            # print(f"{key}: {value}")
            if key == "Thread_block_limit_SM":
                Thread_block_limit_SM = value
            elif key == "Thread_block_limit_registers":
                Thread_block_limit_registers = value
            elif key == "Thread_block_limit_shared_memory":
                Thread_block_limit_shared_memory = value
            elif key == "Thread_block_limit_warps":
                Thread_block_limit_warps = value
            elif key == "Theoretical_max_active_warps_per_SM":
                Theoretical_max_active_warps_per_SM = value
            elif key == "Theoretical_occupancy":
                Theoretical_occupancy = value
            elif key == "L2_cache_requests":
                L2_cache_requests = value
            elif key == "DRAM_total_transactions":
                DRAM_total_transactions = value
            else:
                print("Error: Unknown key")
                exit(0)
    if DEBUG:
        print("Thread_block_limit_SM: ", Thread_block_limit_SM)
        print("Thread_block_limit_registers: ", Thread_block_limit_registers)
        print("Thread_block_limit_shared_memory: ", Thread_block_limit_shared_memory)
        print("Thread_block_limit_warps: ", Thread_block_limit_warps)
        print("Theoretical_max_active_warps_per_SM: ", Theoretical_max_active_warps_per_SM)
        print("Theoretical_occupancy: ", Theoretical_occupancy)
        print("L2_cache_requests: ", L2_cache_requests)
        print("DRAM_total_transactions: ", DRAM_total_transactions)
    
    # ========================================================
    
    patterns = {
        'L2_cache_hit_rate': r"L2_cache_hit_rate = ([\d\s.]+)",
    }
    
    if rank_num == 0:
        extracted_values = {}
        for key, pattern in patterns.items():
            match = re.search(pattern, content)
            if match:
                extracted_values[key] = float(match.group(1))
            else:
                extracted_values[key] = None
                print("No match found for ", key)
                exit(0)

        for key, value in extracted_values.items():
            # print(f"{key}: {value}")
            if key == "L2_cache_hit_rate":
                L2_cache_hit_rate = value
            else:
                print("Error: Unknown key")
                exit(0)
    
    if DEBUG:
        print("L2_cache_hit_rate: ", L2_cache_hit_rate)
    
    # ========================================================
    
    pattern = r"Unified_L1_cache_hit_rate\[\]: ([\d\s.]+)"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        # print(numbers_str)
        numbers = [float(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for Unified_L1_cache_hit_rate[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process.
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if Unified_L1_cache_hit_rate[curr_process_idx] is not None:
                print(f"Error: Unified_L1_cache_hit_rate[{curr_process_idx}] is already set")
                exit(0)
            else:
                Unified_L1_cache_hit_rate[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("Unified_L1_cache_hit_rate: ", Unified_L1_cache_hit_rate)
    
    # ========================================================
    
    pattern = r"Unified_L1_cache_requests\[\]: ((?:\d+ )+(?:\d+))"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [int(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for Unified_L1_cache_requests[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if Unified_L1_cache_requests[curr_process_idx] is not None:
                print(f"Error: Unified_L1_cache_requests[{curr_process_idx}] is already set")
                exit(0)
            else:
                Unified_L1_cache_requests[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("Unified_L1_cache_requests: ", Unified_L1_cache_requests)
    
    # ========================================================
    
    pattern = r"GMEM_read_requests\[\]: ((?:\d+ )+(?:\d+))"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [int(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for GMEM_read_requests[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if GMEM_read_requests[curr_process_idx] is not None:
                print(f"Error: GMEM_read_requests[{curr_process_idx}] is already set")
                exit(0)
            else:
                GMEM_read_requests[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("GMEM_read_requests: ", GMEM_read_requests)
    
    # ========================================================
    
    pattern = r"GMEM_write_requests\[\]: ((?:\d+ )+(?:\d+))"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [int(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for GMEM_write_requests[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if GMEM_write_requests[curr_process_idx] is not None:
                print(f"Error: GMEM_write_requests[{curr_process_idx}] is already set")
                exit(0)
            else:
                GMEM_write_requests[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("GMEM_write_requests: ", GMEM_write_requests)
    
    # ========================================================
    
    pattern = r"GMEM_total_requests\[\]: ((?:\d+ )+(?:\d+))"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [int(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for GMEM_total_requests[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if GMEM_total_requests[curr_process_idx] is not None:
                print(f"Error: GMEM_total_requests[{curr_process_idx}] is already set")
                exit(0)
            else:
                GMEM_total_requests[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("GMEM_total_requests: ", GMEM_total_requests)
    
    # ========================================================
    
    pattern = r"GMEM_read_transactions\[\]: ((?:\d+ )+(?:\d+))"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [int(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for GMEM_read_transactions[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if GMEM_read_transactions[curr_process_idx] is not None:
                print(f"Error: GMEM_read_transactions[{curr_process_idx}] is already set")
                exit(0)
            else:
                GMEM_read_transactions[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("GMEM_read_transactions: ", GMEM_read_transactions)
    
    # ========================================================
    
    pattern = r"GMEM_write_transactions\[\]: ((?:\d+ )+(?:\d+))"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [int(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for GMEM_write_transactions[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if GMEM_write_transactions[curr_process_idx] is not None:
                print(f"Error: GMEM_write_transactions[{curr_process_idx}] is already set")
                exit(0)
            else:
                GMEM_write_transactions[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("GMEM_write_transactions: ", GMEM_write_transactions)
    
    # ========================================================
    
    pattern = r"GMEM_total_transactions\[\]: ((?:\d+ )+(?:\d+))"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [int(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for GMEM_total_transactions[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if GMEM_total_transactions[curr_process_idx] is not None:
                print(f"Error: GMEM_total_transactions[{curr_process_idx}] is already set")
                exit(0)
            else:
                GMEM_total_transactions[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("GMEM_total_transactions: ", GMEM_total_transactions)
    
    # ========================================================
    
    pattern = r"Number_of_read_transactions_per_read_requests\[\]: ((?:\d+ )+(?:\d+))"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [int(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for Number_of_read_transactions_per_read_requests[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if Number_of_read_transactions_per_read_requests[curr_process_idx] is not None:
                print(f"Error: Number_of_read_transactions_per_read_requests[{curr_process_idx}] is already set")
                exit(0)
            else:
                Number_of_read_transactions_per_read_requests[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("Number_of_read_transactions_per_read_requests: ", Number_of_read_transactions_per_read_requests)
    
    # ========================================================
    
    pattern = r"Number_of_write_transactions_per_write_requests\[\]: ((?:\d+ )+(?:\d+))"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [int(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for Number_of_write_transactions_per_write_requests[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if Number_of_write_transactions_per_write_requests[curr_process_idx] is not None:
                print(f"Error: Number_of_write_transactions_per_write_requests[{curr_process_idx}] is already set")
                exit(0)
            else:
                Number_of_write_transactions_per_write_requests[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("Number_of_write_transactions_per_write_requests: ", Number_of_write_transactions_per_write_requests)
    
    # ========================================================
    
    pattern = r"L2_read_transactions\[\]: ((?:\d+ )+(?:\d+))"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [int(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for L2_read_transactions[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if L2_read_transactions[curr_process_idx] is not None:
                print(f"Error: L2_read_transactions[{curr_process_idx}] is already set")
                exit(0)
            else:
                L2_read_transactions[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("L2_read_transactions: ", L2_read_transactions)
    
    # ========================================================
    
    pattern = r"L2_write_transactions\[\]: ((?:\d+ )+(?:\d+))"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [int(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for L2_write_transactions[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if L2_write_transactions[curr_process_idx] is not None:
                print(f"Error: L2_write_transactions[{curr_process_idx}] is already set")
                exit(0)
            else:
                L2_write_transactions[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("L2_write_transactions: ", L2_write_transactions)
    
    # ========================================================
    
    pattern = r"L2_total_transactions\[\]: ((?:\d+ )+(?:\d+))"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [int(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for L2_total_transactions[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if L2_total_transactions[curr_process_idx] is not None:
                print(f"Error: L2_total_transactions[{curr_process_idx}] is already set")
                exit(0)
            else:
                L2_total_transactions[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("L2_total_transactions: ", L2_total_transactions)
    
    # ========================================================
    
    pattern = r"Total_number_of_global_atomic_requests\[\]: ((?:\d+ )+(?:\d+))"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [int(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for Total_number_of_global_atomic_requests[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if Total_number_of_global_atomic_requests[curr_process_idx] is not None:
                print(f"Error: Total_number_of_global_atomic_requests[{curr_process_idx}] is already set")
                exit(0)
            else:
                Total_number_of_global_atomic_requests[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("Total_number_of_global_atomic_requests: ", Total_number_of_global_atomic_requests)
    
    # ========================================================
    
    pattern = r"Total_number_of_global_reduction_requests\[\]: ((?:\d+ )+(?:\d+))"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [int(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for Total_number_of_global_reduction_requests[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if Total_number_of_global_reduction_requests[curr_process_idx] is not None:
                print(f"Error: Total_number_of_global_reduction_requests[{curr_process_idx}] is already set")
                exit(0)
            else:
                Total_number_of_global_reduction_requests[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("Total_number_of_global_reduction_requests: ", Total_number_of_global_reduction_requests)
    
    # ========================================================
    
    pattern = r"Global_memory_atomic_and_reduction_transactions\[\]: ((?:\d+ )+(?:\d+))"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [int(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for Global_memory_atomic_and_reduction_transactions[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if Global_memory_atomic_and_reduction_transactions[curr_process_idx] is not None:
                print(f"Error: Global_memory_atomic_and_reduction_transactions[{curr_process_idx}] is already set")
                exit(0)
            else:
                Global_memory_atomic_and_reduction_transactions[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("Global_memory_atomic_and_reduction_transactions: ", Global_memory_atomic_and_reduction_transactions)
    
    # ========================================================
    
    pattern = r"Achieved_active_warps_per_SM\[\]: ([\d\s.]+)"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [float(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for Achieved_active_warps_per_SM[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if Achieved_active_warps_per_SM[curr_process_idx] is not None:
                print(f"Error: Achieved_active_warps_per_SM[{curr_process_idx}] is already set")
                exit(0)
            else:
                Achieved_active_warps_per_SM[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("Achieved_active_warps_per_SM: ", Achieved_active_warps_per_SM)
    
    # ========================================================
    
    pattern = r"Achieved_occupancy\[\]: ([\d\s.]+)"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [float(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for Achieved_occupancy[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if Achieved_occupancy[curr_process_idx] is not None:
                print(f"Error: Achieved_occupancy[{curr_process_idx}] is already set")
                exit(0)
            else:
                Achieved_occupancy[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("Achieved_occupancy: ", Achieved_occupancy)
    
    # ========================================================
    
    pattern = r"GPU_active_cycles\[\]: ((?:\d+ )+(?:\d+))"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [int(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for GPU_active_cycles[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if GPU_active_cycles[curr_process_idx] is not None:
                print(f"Error: GPU_active_cycles[{curr_process_idx}] is already set")
                exit(0)
            else:
                GPU_active_cycles[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("GPU_active_cycles: ", GPU_active_cycles)
    
    # ========================================================
    
    pattern = r"SM_active_cycles\[\]: ((?:\d+ )+(?:\d+))"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [int(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for SM_active_cycles[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if SM_active_cycles[curr_process_idx] is not None:
                print(f"Error: SM_active_cycles[{curr_process_idx}] is already set")
                exit(0)
            else:
                SM_active_cycles[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("SM_active_cycles: ", SM_active_cycles)
    
    # ========================================================
    
    pattern = r"Warp_instructions_executed\[\]: ((?:\d+ )+(?:\d+))"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [int(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for Warp_instructions_executed[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if Warp_instructions_executed[curr_process_idx] is not None:
                print(f"Error: Warp_instructions_executed[{curr_process_idx}] is already set")
                exit(0)
            else:
                Warp_instructions_executed[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("Warp_instructions_executed: ", Warp_instructions_executed)
    
    # ========================================================
    
    pattern = r"Instructions_executed_per_clock_cycle_IPC\[\]: ((?:[\d.-]+(?:e-)?\d*\s*)+)"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [float(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for Instructions_executed_per_clock_cycle_IPC[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if Instructions_executed_per_clock_cycle_IPC[curr_process_idx] is not None:
                print(f"Error: Instructions_executed_per_clock_cycle_IPC[{curr_process_idx}] is already set")
                exit(0)
            else:
                Instructions_executed_per_clock_cycle_IPC[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("Instructions_executed_per_clock_cycle_IPC: ", Instructions_executed_per_clock_cycle_IPC)
    
    # ========================================================
    
    pattern = r"Total_instructions_executed_per_seconds\[\]: ((?:[\d.-]+(?:e-)?\d*\s*)+)"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [float(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for Total_instructions_executed_per_seconds[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if Total_instructions_executed_per_seconds[curr_process_idx] is not None:
                print(f"Error: Total_instructions_executed_per_seconds[{curr_process_idx}] is already set")
                exit(0)
            else:
                Total_instructions_executed_per_seconds[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("Total_instructions_executed_per_seconds: ", Total_instructions_executed_per_seconds)
    
    # ========================================================
    
    pattern = r"Kernel_execution_time\[\]: ((?:\d+ )+(?:\d+))"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [int(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for Kernel_execution_time[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if Kernel_execution_time[curr_process_idx] is not None:
                print(f"Error: Kernel_execution_time[{curr_process_idx}] is already set")
                exit(0)
            else:
                Kernel_execution_time[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("Kernel_execution_time: ", Kernel_execution_time)
    
    # ========================================================
    
    pattern = r"Simulation_time_memory_model\[\]: ((?:[\d.-]+(?:e-)?\d*\s*)+)"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [float(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for Simulation_time_memory_model[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if Simulation_time_memory_model[curr_process_idx] is not None:
                print(f"Error: Simulation_time_memory_model[{curr_process_idx}] is already set")
                exit(0)
            else:
                Simulation_time_memory_model[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("Simulation_time_memory_model: ", Simulation_time_memory_model)
    
    # ========================================================

    pattern = r"Simulation_time_compute_model\[\]: ((?:[\d.-]+(?:e-)?\d*\s*)+)"
    match = re.search(pattern, content)

    if match:
        numbers_str = match.group(1).strip()
        numbers = [float(n) for n in numbers_str.split()]
        # print(numbers)
    else:
        print("No match found for Simulation_time_compute_model[]")
        exit(0)
    
    # process_idx calculates the index corresponding to the current file/process..
    for pass_num in range(int((SMs_num + all_ranks_num - 1) / all_ranks_num)):
        curr_process_idx = rank_num + pass_num * all_ranks_num
        if curr_process_idx < SMs_num:
            if Simulation_time_compute_model[curr_process_idx] is not None:
                print(f"Error: Simulation_time_compute_model[{curr_process_idx}] is already set")
                exit(0)
            else:
                # print("numbers[{curr_process_idx}]: ", curr_process_idx, numbers[curr_process_idx])
                Simulation_time_compute_model[curr_process_idx] = numbers[curr_process_idx]
    if DEBUG:
        print("Simulation_time_compute_model: ", Simulation_time_compute_model)

    # ========================================================
    

# ========================================================
Unified_L1_cache_hit_rate_summary = 0.0
Unified_L1_cache_requests_summary = 0

for item in Unified_L1_cache_requests:
    if item is not None:
        Unified_L1_cache_requests_summary += item

Unified_L1_cache_hit_requests = 0

for i in range(SMs_num):
    if Unified_L1_cache_hit_rate[i] is not None and Unified_L1_cache_requests[i] is not None:
        Unified_L1_cache_hit_requests += Unified_L1_cache_hit_rate[i] * Unified_L1_cache_requests[i]
        
if Unified_L1_cache_requests_summary != 0:
    Unified_L1_cache_hit_rate_summary = Unified_L1_cache_hit_requests / Unified_L1_cache_requests_summary
# ========================================================
GMEM_read_requests_summary = 0
GMEM_write_requests_summary = 0
GMEM_total_requests_summary = 0
GMEM_read_transactions_summary = 0
GMEM_write_transactions_summary = 0
GMEM_total_transactions_summary = 0

for i in range(SMs_num):
    if GMEM_read_requests[i] is not None:
        GMEM_read_requests_summary += GMEM_read_requests[i]
    if GMEM_write_requests[i] is not None:
        GMEM_write_requests_summary += GMEM_write_requests[i]
    if GMEM_total_requests[i] is not None:
        GMEM_total_requests_summary += GMEM_total_requests[i]
    if GMEM_read_transactions[i] is not None:
        GMEM_read_transactions_summary += GMEM_read_transactions[i]
    if GMEM_write_transactions[i] is not None:
        GMEM_write_transactions_summary += GMEM_write_transactions[i]
    if GMEM_total_transactions[i] is not None:
        GMEM_total_transactions_summary += GMEM_total_transactions[i]
# ========================================================
Number_of_read_transactions_per_read_requests_summary = \
  float(float(GMEM_read_transactions_summary) / float(GMEM_read_requests_summary))
Number_of_write_transactions_per_write_requests_summary = \
  float(float(GMEM_write_transactions_summary) / float(GMEM_write_requests_summary))
# ========================================================
L2_read_transactions_summary = 0
L2_write_transactions_summary = 0
L2_total_transactions_summary = 0

for i in range(SMs_num):
    if L2_read_transactions[i] is not None:
        L2_read_transactions_summary += L2_read_transactions[i]
    if L2_write_transactions[i] is not None:
        L2_write_transactions_summary += L2_write_transactions[i]
    if L2_total_transactions[i] is not None:
        L2_total_transactions_summary += L2_total_transactions[i]
# ========================================================
Total_number_of_global_atomic_requests_summary = 0
Total_number_of_global_reduction_requests_summary = 0
Global_memory_atomic_and_reduction_transactions_summary = 0

for i in range(SMs_num):
    if Total_number_of_global_atomic_requests[i] is not None:
        Total_number_of_global_atomic_requests_summary += \
            Total_number_of_global_atomic_requests[i]
    if Total_number_of_global_reduction_requests[i] is not None:
        Total_number_of_global_reduction_requests_summary += \
            Total_number_of_global_reduction_requests[i]
    if Global_memory_atomic_and_reduction_transactions[i] is not None:
        Global_memory_atomic_and_reduction_transactions_summary += \
            Global_memory_atomic_and_reduction_transactions[i]
# ========================================================
Achieved_active_warps_per_SM_summary = 0
Achieved_occupancy_summary = 0

for i in range(SMs_num):
    if Achieved_active_warps_per_SM[i] is not None:
        Achieved_active_warps_per_SM_summary = \
            max(Achieved_active_warps_per_SM[i], \
                Achieved_active_warps_per_SM_summary)
    if Achieved_occupancy[i] is not None:
        Achieved_occupancy_summary = \
            max(Achieved_occupancy[i], \
                Achieved_occupancy_summary)

if Achieved_active_warps_per_SM_summary > Theoretical_max_active_warps_per_SM:
    Achieved_active_warps_per_SM = Theoretical_max_active_warps_per_SM

if Achieved_occupancy_summary > Theoretical_occupancy:
    Achieved_occupancy_summary = Theoretical_occupancy
# ========================================================
GPU_active_cycles_summary = 0
SM_active_cycles_summary = 0

for i in range(SMs_num):
    if GPU_active_cycles[i] is not None:
        GPU_active_cycles_summary = max(GPU_active_cycles[i], GPU_active_cycles_summary)
    if SM_active_cycles[i] is not None:
        SM_active_cycles_summary = max(SM_active_cycles[i], SM_active_cycles_summary)
# ========================================================
Kernel_execution_time_summary = 0
Simulation_time_memory_model_summary = 0
Simulation_time_compute_model_summary = 0

for i in range(SMs_num):
    if Kernel_execution_time[i] is not None:
        Kernel_execution_time_summary = \
            max(Kernel_execution_time[i], Kernel_execution_time_summary)
    if Simulation_time_memory_model[i] is not None:
        Simulation_time_memory_model_summary = \
            max(Simulation_time_memory_model[i], Simulation_time_memory_model_summary)
    if Simulation_time_compute_model[i] is not None:
        Simulation_time_compute_model_summary = \
            max(Simulation_time_compute_model[i], Simulation_time_compute_model_summary)
# ========================================================
### MAY ERROR
Warp_instructions_executed_summary = 0

for i in range(SMs_num):
    if Warp_instructions_executed[i] is not None:
        Warp_instructions_executed_summary += Warp_instructions_executed[i]

Warp_instructions_executed_summary *= SMs_num

Instructions_executed_per_clock_cycle_IPC_summary = \
    float(Warp_instructions_executed_summary) / float(GPU_active_cycles_summary * SMs_num)

Total_instructions_executed_per_seconds_summary = \
    float(Warp_instructions_executed_summary) / float(Kernel_execution_time_summary * SMs_num) * 1024
# ========================================================

# ========================================================
# Finally, write the summarized content into a new file.
with open(reports_dir + '/' + f'kernel-{kernel_id}-summary.txt', 'w') as f:
    f.write("Summary:\n")
    f.write("\n")
    print("Reports have been written to " + \
          reports_dir + '/' + f'kernel-{kernel_id}-summary.txt')
    f.write(" - Config: " + reports_dir + "\n")
    f.write("\n")
    f.write(" - Theoretical Performance: "+"\n")
    f.write("       * Thread_block_limit_SM: "+str(Thread_block_limit_SM)+"\n")
    f.write("       * Thread_block_limit_registers: "+str(Thread_block_limit_registers)+"\n")
    f.write("       * Thread_block_limit_shared_memory: "+str(Thread_block_limit_shared_memory)+"\n")
    f.write("       * Thread_block_limit_warps: "+str(Thread_block_limit_warps)+"\n")
    f.write("       * Theoretical_max_active_warps_per_SM: "+str(Theoretical_max_active_warps_per_SM)+"\n")
    f.write("       * Theoretical_occupancy: "+str(format(float(Theoretical_occupancy) / 100.0, '.2f'))+"\n")
    f.write("\n")
    f.write(" - L1 Cache Performance: "+"\n")
    f.write("       * Unified_L1_cache_hit_rate: "+str(format(Unified_L1_cache_hit_rate_summary, '.2f'))+"\n")
    f.write("       * Unified_L1_cache_requests: "+str(Unified_L1_cache_requests_summary)+"\n")
    f.write("\n")
    f.write(" - L2 Cache Performance: "+"\n")
    f.write("       * L2_cache_hit_rate: "+str(format(L2_cache_hit_rate, '.2f'))+"\n")
    f.write("       * L2_cache_requests: "+str(L2_cache_requests)+"\n")
    f.write("       * L2_read_transactions: "+str(L2_read_transactions_summary)+"\n")
    f.write("       * L2_write_transactions: "+str(L2_write_transactions_summary)+"\n")
    f.write("       * L2_total_transactions: "+str(L2_total_transactions_summary)+"\n")
    f.write("\n")
    f.write(" - DRAM Performance: "+"\n")
    f.write("       * DRAM_total_transactions: "+str(DRAM_total_transactions)+"\n")
    f.write("\n")
    f.write(" - Global Memory Performance: "+"\n")
    f.write("       * GMEM_read_requests: "+str(GMEM_read_requests_summary)+"\n")
    f.write("       * GMEM_write_requests: "+str(GMEM_write_requests_summary)+"\n")
    f.write("       * GMEM_total_requests: "+str(GMEM_total_requests_summary)+"\n")
    f.write("       * GMEM_read_transactions: "+str(GMEM_read_transactions_summary)+"\n")
    f.write("       * GMEM_write_transactions: "+str(GMEM_write_transactions_summary)+"\n")
    f.write("       * GMEM_total_transactions: "+str(GMEM_total_transactions_summary)+"\n")
    f.write("       * Number_of_read_transactions_per_read_requests: "+\
        str(format(Number_of_read_transactions_per_read_requests_summary, '.2f'))+"\n")
    f.write("       * Number_of_write_transactions_per_write_requests: "+\
        str(format(Number_of_write_transactions_per_write_requests_summary, '.2f'))+"\n")
    f.write("       * Total_number_of_global_atomic_requests: "+\
        str(Total_number_of_global_atomic_requests_summary)+"\n")
    f.write("       * Total_number_of_global_reduction_requests: "+\
        str(Total_number_of_global_reduction_requests_summary)+"\n")
    f.write("       * Global_memory_atomic_and_reduction_transactions: "+\
        str(Global_memory_atomic_and_reduction_transactions_summary)+"\n")
    f.write("\n")
    f.write(" - SMs Performance: "+"\n")
    f.write("       * Achieved_active_warps_per_SM: "+str(format(Achieved_active_warps_per_SM_summary, '.2f'))+"\n")
    f.write("       * Achieved_occupancy: "+str(format(Achieved_occupancy_summary, '.2f'))+"\n")
    f.write("\n")
    f.write("       * GPU_active_cycles: "+str(GPU_active_cycles_summary)+"\n")
    f.write("       * SM_active_cycles: "+str(SM_active_cycles_summary)+"\n")
    f.write("\n")
    f.write("       * Warp_instructions_executed: "+str(Warp_instructions_executed_summary)+"\n")
    f.write("       * Instructions_executed_per_clock_cycle_IPC: "+\
        str(format(Instructions_executed_per_clock_cycle_IPC_summary, '.2f'))+"\n")
    f.write("       * Total_instructions_executed_per_seconds (MIPS): "+\
        str(format(Total_instructions_executed_per_seconds_summary, '.2f'))+"\n")
    f.write("\n")
    f.write("       * Kernel_execution_time (ns): "+str(Kernel_execution_time_summary)+"\n")
    f.write("\n")
    f.write(" - Simulator Performance: "+"\n")
    f.write("       * Simulation_time_memory_model (s): "+str(format(Simulation_time_memory_model_summary, '.2f'))+"\n")
    f.write("       * Simulation_time_compute_model (s): "+str(format(Simulation_time_compute_model_summary, '.2f'))+"\n")
    f.write("\n")
# ========================================================
    
    