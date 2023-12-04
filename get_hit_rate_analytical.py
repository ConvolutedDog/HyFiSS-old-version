
'''
# Dist	     Refs	   Refs(%)	  Cum_Ref	Cum_Ref(%)
     0	        1	0.00240385	        1	0.00240385
     2	        1	0.00240385	        2	0.00480769
     3	       35	0.08413462	       37	0.08894231
     6	        3	0.00721154	       40	0.09615385
     7	        2	0.00480769	       42	0.10096154
     8	        1	0.00240385	       43	0.10336538
     9	       41	0.09855769	       84	0.20192308
    11	        5	0.01201923	       89	0.21394231
    12	        2	0.00480769	       91	0.21875000
    14	        1	0.00240385	       92	0.22115385
#OVFL 	        0	0.00000000	       92	0.22115385
#INF  	      324	0.77884615	      416	1.00000000

     0	        1	0.00240385	        1	0.00240385
     2	        1	0.00240385	        2	0.00480769
     3	       35	0.08413462	       37	0.08894231
     6	        3	0.00721154	       40	0.09615385
     7	        2	0.00480769	       42	0.10096154
     8	        1	0.00240385	       43	0.10336538
     9	       41	0.09855769	       84	0.20192308
    11	        5	0.01201923	       89	0.21394231
    12	        2	0.00480769	       91	0.21875000
    14	        1	0.00240385	       92	0.22115385
    -1	      324	0.77884615	      416	1.00000000
'''

import os, sys, math, time
from scipy import special as sp

reuse_profile = open("./traces/vectoradd/kernel_0_SM_0.histogram",'r').read().strip().split('\n')
reuse_profile = open("./traces/cublas_GemmEx_HF_CC/512x512x512/kernel_0_SM_0.histogram",'r').read().strip().split('\n')
reuse_profile = open("./traces/b+tree/kernel_1_SM_0.histogram",'r').read().strip().split('\n')
reuse_profile = open("./traces/cusparse_spmm_csr_HF_CC/kernel_0_SM_0.histogram",'r').read().strip().split('\n')

need_to_delete = []
for i in range(len(reuse_profile)):
    line = reuse_profile[i]
    if line.startswith("# Dist") or line.startswith("#OVFL"):
        need_to_delete.append(line)
    elif line.startswith("#INF"):
        line = line.replace("#INF", "-1")
        tmp_line = line.split()
        reuse_profile[i] = tmp_line[0] + ", " + tmp_line[2] + ", " + tmp_line[1]
    else:
        tmp_line = line.split()
        reuse_profile[i] = tmp_line[0] + ", " + tmp_line[2] + ", " + tmp_line[1]

for line in need_to_delete:
    reuse_profile.remove(line)

# for line in reuse_profile:
#     print(line)

def qfunc(arg):
    return 0.5-0.5*sp.erf(arg/1.41421)

def get_hit_rate_analytical(reuse_profile, cache_size, line_size, associativity):
    '''
    return the hit rate given RP, cache size, line size, and associativity
    '''
    phit = 0.0 ## Sum (probability of stack distance * probability of hit given D)
    num_blocks = (1.0 * cache_size)/line_size  ## B = Num of Blocks (cache_size/line_size)
    for items in reuse_profile:
        if not items:
            continue
        items = items.split(",")
        stack_distance = int(items[0])
        probability = float(items[1])
        # try:
        #     probability = float(items[1])
        # except:
        #     probability = 0
        ## compute probability of a hit given D
        if stack_distance == -1:   phit += 0
        elif stack_distance == 0:  phit += probability
        else:
            mean = stack_distance * (associativity/num_blocks)
            variance = mean * ((num_blocks-associativity)/num_blocks)
            phit += probability * ( 1 - qfunc( abs(associativity-1-mean) / math.sqrt(variance) ) )

    return phit

phit = get_hit_rate_analytical(reuse_profile, 32*1024, 32, 64)

print("PHIT: ", phit)