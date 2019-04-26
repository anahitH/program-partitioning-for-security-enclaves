import os
import json
from json import JSONEncoder
import re
from pprint import pprint
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from pprint import pprint
import matplotlib.ticker as ticker

STAT_DIR='/home/anahitik/TUM/Thesis/program-partitioning-for-security-enclaves/eval/partition-out'
STAT_FILE='partition_stats.json'
CALL_STACK_FILE='shadow_call_stack.txt'
TEX_OUT_FOLDER='tables'
PARTION_FUNCTIONS_KEY='partition_functions'
SECURE_PARTITION_KEY='secure_partition'
INSECURE_PARTITION_KEY='insecure_partition'

class CallStackEntry:
    def __init__(self, caller, callee, call_num):
        self.caller = caller
        self.callee = callee
        self.call_num = call_num
    
class CallStackEntryEncoder(JSONEncoder):
    def default(self, object):
        if isinstance(object, CallStackEntry):
            return object.__dict__
        else:
            # call base class implementation which takes care of
            # raising exceptions for unsupported types
            return json.JSONEncoder.default(self, object)

# programs the evaluation was ran for
programs = []
# coverage percentage - corresponds to subdirectories in program directory
coverages = []
#optimizations - subdirectories in each coverage dir
optimizations = []
secure_partition = []
insecure_partition = []
# map {program : {coverage : {opt : ctx_switches}}}
context_switches = {}
call_stack_entries = {}

def get_bitcode_name_from_path(dir_name):
    return os.path.basename(os.path.normpath(dir_name))

def get_immediate_subdirectories(a_dir):
    return [name for name in os.listdir(a_dir)
            if os.path.isdir(os.path.join(a_dir, name))]

def parse_partition_stats_data(program, coverage, opt):
    stat_file = os.path.join(STAT_DIR, program, coverage, opt, STAT_FILE)
    if not os.path.isfile(stat_file):
        return
    stats = json.load(open(stat_file))
    for secure_f in stats["partition"][SECURE_PARTITION_KEY][PARTION_FUNCTIONS_KEY]:
        secure_partition.append(secure_f)
    for insecure_f in stats["partition"][INSECURE_PARTITION_KEY][PARTION_FUNCTIONS_KEY]:
        insecure_partition.append(insecure_f)

def parse_call_stack_data(program, coverage, opt):
    stack_file = os.path.join(STAT_DIR, program, coverage, opt, CALL_STACK_FILE)
    if not os.path.isfile(stack_file):
        return
    with open(stack_file, 'rb') as call_stack_file:
        stack_line = call_stack_file.readline()
        while stack_line:
            stack_info = filter(None, re.split('\(|\)|\s+', stack_line))
            call_stack_entries[program][coverage][opt].append(CallStackEntry(stack_info[0], stack_info[1], int(stack_info[2])))
            stack_line = call_stack_file.readline()
        

def parse_data_for_program_coverage_optimization(program, coverage, opt):
    parse_partition_stats_data(program, coverage, opt)
    parse_call_stack_data(program, coverage, opt)

def parse_data_for_program_coverage(program, coverage):
    opt_dirs = get_immediate_subdirectories(os.path.join(STAT_DIR, program, coverage))
    for opt_dir in opt_dirs:
        if opt_dir not in optimizations:
            optimizations.append(opt_dir)
        call_stack_entries[program][coverage][opt_dir] = []
        context_switches[program][coverage][opt_dir] = 0
        parse_data_for_program_coverage_optimization(program, coverage, opt_dir)

def parse_data_for_program(program):
    cov_dirs = get_immediate_subdirectories(os.path.join(STAT_DIR, program))
    for cov_dir in cov_dirs:
        if cov_dir not in coverages:
            coverages.append(cov_dir)
        call_stack_entries[program][cov_dir] = {}
        context_switches[program][cov_dir] = {}
        parse_data_for_program_coverage(program, cov_dir)

def parse_data():
    rootDir = STAT_DIR
    directories = get_immediate_subdirectories(rootDir)
    print ("Programs evaluation was ran for ")
    print (directories)
    for program_dir in directories:
        if program_dir not in programs:
            programs.append(program_dir)
        call_stack_entries[program_dir] = {}
        context_switches[program_dir] = {}
        parse_data_for_program(program_dir)

def dump_call_stack_data():
    call_stack = "shadow_call_stack.json"
    with open(call_stack, 'a') as f :
        f.write(json.dumps(call_stack_entries, indent=4, default=lambda x: x.__dict__))

def dump_ctx_switch_data():
    ctx_switch = "context_switch_eval.json"
    with open(ctx_switch, 'a') as f:
        f.write(json.dumps(context_switches, indent=4))

def is_context_switch(caller, callee):
    is_secure_caller = (caller in secure_partition)
    is_secure_callee = (callee in secure_partition)
    return not (is_secure_caller == is_secure_callee)
        
def compute_context_switches():
    for program in call_stack_entries:
        cov_data = call_stack_entries[program]
        for cov in cov_data:
            opt_data = cov_data[cov]
            for opt in opt_data:
                for call_data in opt_data[opt]:
                   if is_context_switch(call_data.caller, call_data.callee):
                        #print ("Context switch in: program: " + program + " cov: " + cov + " opt: " + opt + "- "+ call_data.caller + " " + call_data.callee + " " + str(call_data.call_num))
                        context_switches[program][cov][opt] += call_data.call_num

def get_ctx_switches_for_program(program, optimizations_to_visualize):
    # {opt: [tcb for cov1, tcb for cov2 ..]}
    result = {}
    for coverage in coverages:
        #print (coverage)
        #print ( partition_tcb[program][coverage])
        for opt in optimizations_to_visualize:
            if opt not in result:
                result[opt] = []
            ctx_switch =  context_switches[program][coverage][opt]
            result[opt].append(ctx_switch)
    return result

def prepare_xtick_labels(coverage_labels, programs, gaps, cov_num):
    #add program name to the labels
    modified_coverage_labels = []
    for cov_l in coverage_labels:
        if cov_l.isdigit():
            modified_coverage_labels.append(cov_l + '%')
        else:
            modified_coverage_labels.append(cov_l)
        modified_coverage_labels = (modified_coverage_labels + (['']*gaps * cov_num))
    return modified_coverage_labels

def prepare_yticks(tcbs):
    yticks = []
    max_tcb = 0
    for opt in tcbs:
        max_tcb = max(max_tcb, max(tcbs[opt]))
    step = max_tcb / 5
    value = 0
    while value < max_tcb:
        yticks.append(value + step)
        value = value + step
    return yticks


def visualize_data_for_program_and_cov(program):
    # for each program in evaluation
        # for each coverage percentage
            # no-opt - transparent bar
            # static analysis - black bar
            # ilp - red bar
            # kl - yellow bar
    opt_colors = {}
    opt_colors["no-opt"] = 'w' # white
    opt_colors["static-analysis"] = '#FFFFCC' # grey
    opt_colors["ilp"] = 'r' # red
    opt_colors["kl"] = 'y' # red
    optimizations_to_visualize = ['no-opt', 'static-analysis', 'ilp', 'kl']
    gaps = 1
    width = 0.35
    opt_num = 4
    cov_num = len(coverages)
    ind = np.arange(0, cov_num * (cov_num+gaps) * width, width) #Number of bars we need is in total N (programs) times N (coverages) time N (optimizations)
    rects = []

    ctx_switches_for_program = get_ctx_switches_for_program(program, optimizations_to_visualize)
    print ('Visualize plot for program ' + program)
    #print ("PROGRAM " + program)
    fig, ax = plt.subplots()
    coverage_labels = []
    i = 0
    for opt in optimizations_to_visualize:
        ctx_switches_for_opt = ctx_switches_for_program[opt]
        ax_color = opt_colors[opt]
        columns = ind[i:len(ind)-1:cov_num+gaps]
        rects1 = ax.bar(columns, ctx_switches_for_opt, width, color=ax_color, edgecolor='black', capsize=4, error_kw={'ecolor':'red'},label=optimizations_to_visualize[i])
        i+=1
        rects.append(rects1)
    ax.set_ylabel('TCB for ' + program)
    ax.set_xlabel('Percentages')
    ax.set_xticks(ind + width/2) #np.arange(np.min(ind),np.max(ind)+1, width))
    ax.set_xticklabels(prepare_xtick_labels(coverages, programs, gaps ,cov_num))
    y_ticks = [1000, 2000, 3000, 4000, 5000]
    y_ticks_n = np.array(y_ticks).astype(np.int)
    ax.set_yticks(y_ticks_n)
    #ax.set_yscale('log',basey=2)
    plt.xticks(rotation=60)
    plt.legend(bbox_to_anchor=(0., 1.02, 1., .102),loc='upper right', ncol=4, mode="expand", borderaxespad=0.)
    plt.subplots_adjust(bottom=0.30)
    fig_name = program + "_ctx_switch_eval.png"
    plt.ion()
    fig.set_size_inches(4.7,3.0)
    fig.savefig(fig_name,bbox_inches='tight')


def visualize_data():
    coverages.sort()
    for program in programs:
        visualize_data_for_program_and_cov(program)

def main():
    parse_data()
    compute_context_switches()
    dump_call_stack_data()
    dump_ctx_switch_data()
    visualize_data()

if __name__=="__main__":
    main()


