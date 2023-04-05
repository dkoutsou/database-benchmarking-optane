import pandas as pd
import numpy as np

cols = ['Read', 'Write(nt)', 'Write(clwb)']

# read files
dram_read = {}
dram_clwb = {}
dram_nt = {}

pmem_read = {}
pmem_clwb = {}
pmem_nt = {}

f1 = open('rd_bw_all.txt')
for line in f1.readlines():
    tokens = line.split()
    if (tokens[0] == '1'): # dram
        t = int(tokens[2])
        if (t in dram_read):
            dram_read[t].append(float(tokens[6]))
        else:
            dram_read[t] = [float(tokens[6])]
    else: # pmem
        t = int(tokens[2])
        if (t in pmem_read):
            pmem_read[t].append(float(tokens[6]))
        else:
            pmem_read[t] = [float(tokens[6])]


for t in dram_read.keys():
    dram_read[t] = np.median(dram_read[t])

for t in pmem_read.keys():
    pmem_read[t] = np.median(pmem_read[t])

f2 = open('ntstore_bw_all.txt')
for line in f2.readlines():
    tokens = line.split()
    if (tokens[0] == '1'): # dram
        t = int(tokens[2])
        if (t in dram_nt):
            dram_nt[t].append(float(tokens[6]))
        else:
            dram_nt[t] = [float(tokens[6])]
    else:
        t = int(tokens[2])
        if (t in pmem_nt):
            pmem_nt[t].append(float(tokens[6]))
        else:
            pmem_nt[t] = [float(tokens[6])]


for t in dram_nt.keys():
    dram_nt[t] = np.median(dram_nt[t])

for t in pmem_nt.keys():
    pmem_nt[t] = np.median(pmem_nt[t])

f3 = open('storeclwb_bw_all.txt')
for line in f3.readlines():
    tokens = line.split()
    if (tokens[0] == '1'): # dram
        t = int(tokens[2])
        if (t in dram_clwb):
            dram_clwb[t].append(float(tokens[6]))
        else:
            dram_clwb[t] = [float(tokens[6])]
    else:
        t = int(tokens[2])
        if (t in pmem_clwb):
            pmem_clwb[t].append(float(tokens[6]))
        else:
            pmem_clwb[t] = [float(tokens[6])]


for t in dram_clwb.keys():
    dram_clwb[t] = np.median(dram_clwb[t])

for t in pmem_clwb.keys():
    pmem_clwb[t] = np.median(pmem_clwb[t])

# dram
dram_total = {}
for t in dram_nt:
    dram_total[t] = [dram_read[t], dram_nt[t], dram_clwb[t]]

# pmem
pmem_total = {}
for t in pmem_nt:
    pmem_total[t] = [pmem_read[t], pmem_nt[t], pmem_clwb[t]]

print(dram_total)
df1 = pd.DataFrame.from_dict(dram_total, orient='index', columns=cols)
df1.to_csv('dram.csv', columns=cols)
print(df1)

# pmem

print(pmem_total)
df2 = pd.DataFrame.from_dict(pmem_total, orient='index', columns=cols)
df2.to_csv('pmem.csv', columns=cols)
print(df2)

