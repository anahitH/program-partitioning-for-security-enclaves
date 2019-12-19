from __future__ import division
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
import sys

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
# repeats
repeats = []
#optimizations - subdirectories in each coverage dir
optimizations = []
# program : {coverage : {repeat : {opt : [list of secure partition]}}}
secure_partition = {}
# program : {coverage : {repeat : {opt : [list of insecure partition]}}}
insecure_partition = {}
# map {program : {coverage : {opt : avg_ctx_switches over all repeats}}}
context_switches = {}
# program : [list of call stack entries]
call_stack_entries = {}

partition_coverage_context_switches = {}
# program : {coverage : avg_improvement}
ilp_improvement = {}

def average(numbers):
    import numpy as np
    return round(np.average(numbers), 1)

def median(numbers):
    from numpy import median
    return round(median(numbers), 1)

def std_deviation(numbers):
    import numpy as np
    return round(np.std(numbers), 1)


def get_bitcode_name_from_path(dir_name):
    return os.path.basename(os.path.normpath(dir_name))

def get_immediate_subdirectories(a_dir):
    return [name for name in os.listdir(a_dir)
            if os.path.isdir(os.path.join(a_dir, name))]

def is_context_switch(caller, callee, secure_fs, insecure_fs):
    is_secure_caller = (caller in secure_fs)
    is_secure_callee = (callee in secure_fs)
    return not (is_secure_caller == is_secure_callee)
 
def compute_context_switches():
    for program in programs:
        print ("Compute context switches for " + program)
        program_call_entries = call_stack_entries[program]
        if not program in ilp_improvement:
            ilp_improvement[program] = {}
        #print ("compute context switches for program: " + program)
        for cov in coverages:
            if cov not in partition_coverage_context_switches:
                partition_coverage_context_switches[cov] = {}
            #print ("coverage: " + cov)
            for repeat in repeats:
                #print ("repeat: " + repeat)
                for opt in optimizations:
                    if cov == "expert-knowledge" and repeat != "no-repeat":
                        continue
                    if cov != "expert-knowledge" and repeat == "no-repeat":
                        continue
                    if opt not in partition_coverage_context_switches[cov]:
                        partition_coverage_context_switches[cov][opt] = []
                    secure_fs = secure_partition[program][cov][repeat][opt]
                    insecure_fs = insecure_partition[program][cov][repeat][opt]
                    for call_data in program_call_entries:
                        if is_context_switch(call_data.caller, call_data.callee, secure_fs, insecure_fs):
                            context_switches[program][cov][opt] += call_data.call_num
                            #if program == "mcu" and cov == "25" and opt == "ilp" and context_switches[program][cov]["ilp"] == 0:
                            #    print ("Repeat " + str(repeat) + ": " + str(context_switches[program][cov]["ilp"]))
                    partition_coverage_context_switches[cov][opt].append(context_switches[program][cov][opt])
            #print (cov)
            #print (context_switches[program][cov]["ilp"])
            #print (context_switches[program][cov]["search-based"])
            #if (context_switches[program][cov]["ilp"] == 0):
            #    print ("Stop")
            ilp_improvement[program][cov] = context_switches[program][cov]["search-based"]/context_switches[program][cov]["ilp"]
                
    for program in context_switches:
        program_data = context_switches[program]
        for cov in program_data:
            if (cov == "expert-knowledge"):
                continue
            cov_data = program_data[cov]
            for opt in cov_data:
                cov_data[opt] = cov_data[opt] / (len(repeats) - 1)

def parse_data_for_program_coverage_optimization(program, coverage, repeat_dir, opt_dir):
    stat_file = os.path.join(STAT_DIR, program, coverage, repeat_dir, opt_dir, STAT_FILE)
    if not os.path.isfile(stat_file):
        return
    stats = json.load(open(stat_file))
    for secure_f in stats["partition"][SECURE_PARTITION_KEY][PARTION_FUNCTIONS_KEY]:
        secure_partition[program][coverage][repeat_dir][opt_dir].append(secure_f)
    for insecure_f in stats["partition"][INSECURE_PARTITION_KEY][PARTION_FUNCTIONS_KEY]:
        insecure_partition[program][coverage][repeat_dir][opt_dir].append(insecure_f)

def parse_data_for_expert_knowledge_coverage_optimization(program, coverage, opt_dir):
    stat_file = os.path.join(STAT_DIR, program, coverage, opt_dir, STAT_FILE)
    if not os.path.isfile(stat_file):
        return
    stats = json.load(open(stat_file))
    for secure_f in stats["partition"][SECURE_PARTITION_KEY][PARTION_FUNCTIONS_KEY]:
        secure_partition[program][coverage]["no-repeat"][opt_dir].append(secure_f)
    for insecure_f in stats["partition"][INSECURE_PARTITION_KEY][PARTION_FUNCTIONS_KEY]:
        insecure_partition[program][coverage]["no-repeat"][opt_dir].append(insecure_f)

def parse_data_for_expert_knowledge_coverage(program, coverage):
    opt_dirs = get_immediate_subdirectories(os.path.join(STAT_DIR, program, coverage))
    repeat = 'no-repeat'
    if repeat not in repeats:
        repeats.append(repeat)
    if repeat not in secure_partition:
        secure_partition[program][coverage][repeat] = {}
    if repeat not in insecure_partition:
        insecure_partition[program][coverage][repeat] = {}

    for opt_dir in opt_dirs:
        if opt_dir not in optimizations:
            optimizations.append(opt_dir)
        secure_partition[program][coverage][repeat][opt_dir] = []
        insecure_partition[program][coverage][repeat][opt_dir] = []
        context_switches[program][coverage][opt_dir] = 0
        parse_data_for_expert_knowledge_coverage_optimization(program, coverage, opt_dir)

   
def parse_data_for_program_coverage(program, cov_dir):
    if cov_dir == "expert-knowledge":
        parse_data_for_expert_knowledge_coverage(program, cov_dir)
        return

    repeat_dirs = get_immediate_subdirectories(os.path.join(STAT_DIR, program, cov_dir))
    for repeat_dir in repeat_dirs:
        if repeat_dir not in repeats:
            repeats.append(repeat_dir)
        secure_partition[program][cov_dir][repeat_dir] = {}
        insecure_partition[program][cov_dir][repeat_dir] = {}
        opt_dirs = get_immediate_subdirectories(os.path.join(STAT_DIR, program, cov_dir, repeat_dir))
        for opt_dir in opt_dirs:
            if opt_dir not in optimizations:
                optimizations.append(opt_dir)
            secure_partition[program][cov_dir][repeat_dir][opt_dir] = []
            insecure_partition[program][cov_dir][repeat_dir][opt_dir] = []
            context_switches[program][cov_dir][opt_dir] = 0
            parse_data_for_program_coverage_optimization(program, cov_dir, repeat_dir, opt_dir)

def parse_stat_data(program):
    print ("Parse stat data for " + program)
    cov_dirs = get_immediate_subdirectories(os.path.join(STAT_DIR, program))
    for cov_dir in cov_dirs:
        if cov_dir not in coverages:
            coverages.append(cov_dir)
        context_switches[program][cov_dir] = {}
        secure_partition[program][cov_dir] = {}
        insecure_partition[program][cov_dir] = {}
        parse_data_for_program_coverage(program, cov_dir)

def parse_call_entries(program_dir):
    print ("Parse call entries for " + program_dir)
    stack_file = os.path.join(STAT_DIR, program_dir, CALL_STACK_FILE)
    if not os.path.isfile(stack_file):
        return
    #print (stack_file)
    with open(stack_file, 'r') as call_stack_file:
        for stack_line in call_stack_file:
            #print (stack_line)
            stack_info = filter(None, re.split('\(|\)|\s+', stack_line))
            if (len(stack_info) < 2):
                continue
                #print (program_dir)
                #print (stack_info)
            if (len(stack_info) != 3):
                call_stack_entries[program_dir].append(CallStackEntry("external", stack_info[0], int(stack_info[1])))
            else:
                call_stack_entries[program_dir].append(CallStackEntry(stack_info[0], stack_info[1], int(stack_info[2])))
            #stack_line = call_stack_file.readline()
 
def parse_data():
    print ("parse data")
    rootDir = STAT_DIR
    directories = get_immediate_subdirectories(rootDir)
    print ("Programs evaluation was ran for ")
    print (directories)
    for program_dir in directories:
        if program_dir not in programs:
            programs.append(program_dir)
        call_stack_entries[program_dir] = []
        context_switches[program_dir] = {}
        secure_partition[program_dir] = {}
        insecure_partition[program_dir] = {}
        parse_call_entries(program_dir)
        parse_stat_data(program_dir)

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

def prepare_yticks(context_switches):
    yticks = []
    max_ctx = 0
    for opt in context_switches:
        max_ctx = max(max_ctx, max(context_switches[opt]))
    step = max_ctx / 5
    value = 0
    while value < max_ctx:
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
    opt_colors["search-based"] = 'r' # red
    opt_colors["ilp"] = 'y' # yellow
    opt_colors["kl"] = 'b' # yellow
    optimizations_to_visualize = ['no-opt', 'search-based', 'kl', 'ilp']
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
    ax.set_ylabel('Context switches for ' + program)
    ax.set_xlabel('Percentages')
    ax.set_xticks(ind + width/2) #np.arange(np.min(ind),np.max(ind)+1, width))
    ax.set_xticklabels(prepare_xtick_labels(coverages, programs, gaps ,cov_num))
    #y_ticks = [1000, 2000, 3000, 4000, 5000]
    y_ticks = prepare_yticks(ctx_switches_for_program)
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

def dump_tex_tables():
    from tabulate import tabulate
    tabulate.LATEX_ESCAPE_RULES={}
    table_headers = ["", "10%", "25%", "35%", "50%", "Expert knowledge"]
    ilp_coverage_table_data = []
    kl_coverage_table_data = []
    search_based_coverage_table_data = []

    ilp_mean = []
    ilp_median = []
    ilp_std = []

    kl_mean = []
    kl_median = []
    kl_std = []

    search_based_mean = []
    search_based_median = []
    search_based_std = []
    
    ilp_mean.append('ilp mean')
    ilp_median.append('ilp median')
    ilp_std.append('ilp std. dev.')
    
    kl_mean.append('kl mean')
    kl_median.append('kl median')
    kl_std.append('kl std. dev.')
   
    search_based_mean.append('search based mean')
    search_based_median.append('search based median')
    search_based_std.append('search based std. dev.')
  
    for cov_tcb in partition_coverage_context_switches:
        for opt_dir in partition_coverage_context_switches[cov_tcb]:
            mean = average(partition_coverage_context_switches[cov_tcb][opt_dir])
            med = median(partition_coverage_context_switches[cov_tcb][opt_dir])
            std_dev = std_deviation(partition_coverage_context_switches[cov_tcb][opt_dir])
            if opt_dir == 'ilp':
                ilp_mean.append(mean);
                ilp_median.append(med);
                ilp_std.append(std_dev);
            if opt_dir == 'search-based':
                search_based_mean.append(mean);
                search_based_median.append(med);
                search_based_std.append(std_dev);
            if opt_dir == 'kl':
                kl_mean.append(mean);
                kl_median.append(med);
                kl_std.append(std_dev);

    ilp_coverage_table_data.append(ilp_mean)
    ilp_coverage_table_data.append(ilp_median)
    ilp_coverage_table_data.append(ilp_std)

    kl_coverage_table_data.append(kl_mean)
    kl_coverage_table_data.append(kl_median)
    kl_coverage_table_data.append(kl_std)

    search_based_coverage_table_data.append(search_based_mean)
    search_based_coverage_table_data.append(search_based_median)
    search_based_coverage_table_data.append(search_based_std)

    ilp_table = tabulate(ilp_coverage_table_data, headers=table_headers, tablefmt="latex")
    kl_table = tabulate(kl_coverage_table_data, headers=table_headers, tablefmt="latex")
    search_based_table = tabulate(search_based_coverage_table_data, headers=table_headers, tablefmt="latex")
    table_file_name = "context_switches_eval_tables.tex"

    #table_file = os.path.join(TEX_OUT_FOLDER,"paper_tables.tex")
    table_file = os.path.join(TEX_OUT_FOLDER, table_file_name)
    with open(table_file,'wb') as tablefile:
        tablefile.write(ilp_table)
        tablefile.write(kl_table)
        tablefile.write(search_based_table)


def dump_ctx_switch_data():
    ctx_switch = "context_switch_eval.json"
    cov_ctx_mean = {}
    cov_ctx_median = {}
    cov_ctx_std = {}
    for cov_ctx in partition_coverage_context_switches:
        cov_ctx_mean[cov_ctx] = {}
        cov_ctx_median[cov_ctx] = {}
        cov_ctx_std[cov_ctx] = {}
        for opt_dir in partition_coverage_context_switches[cov_ctx]:
            cov_ctx_mean[cov_ctx][opt_dir] = average(partition_coverage_context_switches[cov_ctx][opt_dir])
            cov_ctx_median[cov_ctx][opt_dir] = median(partition_coverage_context_switches[cov_ctx][opt_dir])
            cov_ctx_std[cov_ctx][opt_dir] = std_deviation(partition_coverage_context_switches[cov_ctx][opt_dir])

    with open(ctx_switch, 'a') as f:
        f.write(json.dumps({"programs_context_switches" : context_switches}, indent=4))
        f.write(json.dumps({"coverage_mean" : cov_ctx_mean}, indent = 4))
        f.write(json.dumps({"coverage_median" : cov_ctx_median}, indent = 4))
        f.write(json.dumps({"coverage_std" : cov_ctx_std}, indent = 4))
        f.write(json.dumps({"ilp_improvement" : ilp_improvement} , indent = 4))


def main():
    parse_data()
    #print ("secure partition")
    #print (secure_partition)
    #print ("insecure partition")
    #print (insecure_partition)
    compute_context_switches()
    #dump_call_stack_data()
    dump_ctx_switch_data()
    dump_tex_tables()
    visualize_data()
    #print ("context switches")

if __name__=="__main__":
    main()


