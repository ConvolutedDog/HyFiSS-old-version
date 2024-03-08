#!/bin/python
# -*- coding: utf-8 -*-

import os
import argparse

'''
format of the input file kernel_\d+.sass:
0 ! 96 0 ! 137 0 ! 10b 0 ! 136 0 ! fd 0 ! fb 0 ! 69 0 ! 11f 0 ! 92 0 ! 6b 0 ! 5d 0 ! f3 0 ! 91 0 ! 51 0 ! 5e 0 ! 133 0 ! 131 0 ! b3 0 ! 8a 0 ! cb 0 ! 122 0 ! 29 0 ! 9b 0 ! 2b 0 ! 10f 0 ! 8c 0 ! 102 0 ! 8e 0 ! 103 0 ! 6c 0 ! 6e 0 ! 2c 0 ! 4b 0 ! be 0 ! b6 0 ! d3 0 ! 56 0 ! b5 0 ! d8 0 ! 118 0 ! 14 0 ! db 0 ! 13d 0 ! 58 0 ! 39 0 ! 113 0 ! 16 0 ! 125 0 ! 11a 0 ! 3d 0 ! 59 0 ! 5b 0 ! 26 0 ! 31 0 ! 1e 0 ! 138 0 ! 3c 0 ! 81 0 ! 3e 0 ! 83 0 ! 8 0 ! 9 0 ! 13e 0 ! ac 0 ! 127 0 ! 116 0 ! 13b 0 ! 54 0 ! 79 0 ! af 0 ! 13a 0 ! 2d 0 ! 7a 0 ! 115 0 ! 3b 0 ! b8 0 ! 117 0 ! 38 0 ! f6 0 ! f5 0 ! 86 0 ! bb

Every three hexadecimal strings are a set and are a triplet (PC, mask, gwarp id), where mask="!" 
means that mask=0xffffffff. Now use Python to write triples belonging to different gwarp ids to 
different files, named Kernel_d+_gwarp_id_\d+.sass.
'''

parser = argparse.ArgumentParser(description='Process sass dir.')

parser.add_argument('--dir', type=str, required=True,
                    help='The directory of sass files')

args = parser.parse_args()

sass_dir = args.dir
sass_dir = os.path.abspath(sass_dir)

# read the sass files in sass_dir
files = os.listdir(sass_dir)
sass_files = [os.path.join(sass_dir, file) for file in files if file.endswith(".sass")]

for sass_file in sass_files:
    content = open(sass_file, "r").read().split(" ")
    kernel_id = int(sass_file.split("/")[-1].split("_")[1].split(".sass")[0])
    
    have_created_gwarp_ids = []
    for i in range(int(len(content)/3)):
        gwarp_id = int(content[i*3+2], 16)
        if gwarp_id in have_created_gwarp_ids:
            pass
        else:
            have_created_gwarp_ids.append(gwarp_id)
            if not os.path.exists(os.path.join(sass_dir, "kernel_"+str(kernel_id)+"_gwarp_id_"+str(gwarp_id)+".split.sass")):
                open(os.path.join(sass_dir, "kernel_"+str(kernel_id)+"_gwarp_id_"+str(gwarp_id)+".split.sass"), "w").close()
        with open(os.path.join(sass_dir, "kernel_"+str(kernel_id)+"_gwarp_id_"+str(gwarp_id)+".split.sass"), "a") as f:
            f.write(content[i*3]+" "+content[i*3+1]+"\n")
    