# Use this to generate a memory trace file from a compiled binary.
# Requires valgrind to run.

import sys, subprocess

prog_name = sys.argv[1]
command = "valgrind --tool=lackey --trace-mem=yes --basic-counts=no ./" + prog_name

result = subprocess.Popen(command, stderr=subprocess.PIPE, shell=True)
out = result.communicate()[1].decode('utf-8')

lines = out.split(sep='\n')
trace = []
for i in range(len(lines)):
    if 'L ' in lines[i]:
        instr_addr = lines[i - 1][3:11] 
        mem_addr = lines[i][3:11]
        trace_line = "0x" + instr_addr + ": R 0x" + mem_addr
        trace.append(trace_line)
    if 'S ' in lines[i]:
        instr_addr = lines[i - 1][3:11] 
        mem_addr = lines[i][3:11]
        trace_line = "0x" + instr_addr + ": W 0x" + mem_addr
        trace.append(trace_line)
        
trace_file = open("trace.txt", "w")
for t in trace:
    trace_file.write(t + "\n")
trace_file.write("#eof\n")
