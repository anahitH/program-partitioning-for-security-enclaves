import os
import json
from pprint import pprint
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from pprint import pprint
import matplotlib.ticker as ticker

STAT_DIR="/home/anahitik/TUM/Thesis/program-partitioning-for-security-enclaves/eval/partition-out"
STAT_FILE="partition_stats.json"
TEX_OUT_FOLDER='tables'
PARTITION_PERCENT_KEY="partition%"
TCB_SIZE_KEY="TCB"
ARGS_PASSED_KEY="args_passed"

# programs the evaluation was ran for
programs = []
# coverage percentage - corresponds to subdirectories in program directory
coverages = []
#optimizations - subdirectories in each coverage dir
optimizations = []
# map {program : {coverage : {opt : tcb}}}
partition_tcb = {}

def get_bitcode_name_from_path(dir_name):
    return os.path.basename(os.path.normpath(dir_name))

def get_immediate_subdirectories(a_dir):
    return [name for name in os.listdir(a_dir)
            if os.path.isdir(os.path.join(a_dir, name))]

def get_tcb_for_program(program, optimizations_to_visualize):
    # {opt: [tcb for cov1, tcb for cov2 ..]}
    result = {}
    for coverage in coverages:
        #print (coverage)
        #print ( partition_tcb[program][coverage])
        for opt in optimizations_to_visualize:
            if opt not in result:
                result[opt] = []
            tcb =  partition_tcb[program][coverage][opt]
            result[opt].append(tcb)
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

    tcb_for_program = get_tcb_for_program(program, optimizations_to_visualize)
    print ('Visualize plot for program ' + program)
    #print ("PROGRAM " + program)
    #print ("tcb_for_program are:")
    #print (tcb_for_program)
    fig, ax = plt.subplots()
    coverage_labels = []
    i = 0
    for opt in optimizations_to_visualize:
        tcbs_for_opt = tcb_for_program[opt]
        ax_color = opt_colors[opt]
        columns = ind[i:len(ind)-1:cov_num+gaps]
        rects1 = ax.bar(columns, tcbs_for_opt, width, color=ax_color, edgecolor='black', capsize=4, error_kw={'ecolor':'red'},label=optimizations_to_visualize[i])
        i+=1
        rects.append(rects1)
    ax.set_ylabel('TCB for ' + program)
    ax.set_xlabel('Percentages')
    ax.set_xticks(ind + width/2) #np.arange(np.min(ind),np.max(ind)+1, width))
    ax.set_xticklabels(prepare_xtick_labels(coverages, programs, gaps ,cov_num))
    #y_ticks = prepare_yticks(tcb_for_program)
    y_ticks = [100, 300, 500, 700, 900]
    y_ticks_n = np.array(y_ticks).astype(np.int)
    ax.set_yticks(y_ticks_n)
    #ax.set_yscale('log',basey=2)
    plt.xticks(rotation=60)
    plt.legend(bbox_to_anchor=(0., 1.02, 1., .102),loc='upper right', ncol=4, mode="expand", borderaxespad=0.)
    plt.subplots_adjust(bottom=0.30)
    fig_name = program + "_tcb_eval.png"
    plt.ion()
    fig.set_size_inches(4.7,3.0)
    fig.savefig(fig_name,bbox_inches='tight')

        
def visualize_data():
    coverages.sort()
    for program in programs:
        visualize_data_for_program_and_cov(program)


def dump_data():
    coverage_file = "secure_tcb_eval.json"
    with open(coverage_file, 'a') as f :
        json.dump(partition_tcb, f)

def parse_data_for_program_coverage_optimization(program, coverage, opt):
    stat_file = os.path.join(STAT_DIR, program, coverage, opt, STAT_FILE)
    #if program not in partition_tcb:
    #    partition_tcb[program] = {}
    #if coverage not in partition_tcb[program]:
    #    partition_tcb[program][coverage] = {}
    #if opt not in partition_tcb[program][coverage]:
    #    partition_tcb[program][coverage][opt] = 0
    if not os.path.isfile(stat_file):
        return
    stats = json.load(open(stat_file))
    partition_tcb[program][coverage][opt] = stats["partition"]["secure_partition"][TCB_SIZE_KEY]

def parse_data_for_program_coverage(program, coverage):
    opt_dirs = get_immediate_subdirectories(os.path.join(STAT_DIR, program, coverage))
    for opt_dir in opt_dirs:
        if opt_dir not in optimizations:
            optimizations.append(opt_dir)
        partition_tcb[program][coverage][opt_dir] = 0
        parse_data_for_program_coverage_optimization(program, coverage, opt_dir)

def parse_data_for_program(program):
    cov_dirs = get_immediate_subdirectories(os.path.join(STAT_DIR, program))
    for cov_dir in cov_dirs:
        if cov_dir not in coverages:
            coverages.append(cov_dir)
        partition_tcb[program][cov_dir] = {}
        parse_data_for_program_coverage(program, cov_dir)

def parse_data():
    rootDir = STAT_DIR
    directories = get_immediate_subdirectories(rootDir)
    print ("Programs evaluation was ran for ")
    print (directories)
    for program_dir in directories:
        if program_dir not in programs:
            programs.append(program_dir)
        partition_tcb[program_dir] = {}
        parse_data_for_program(program_dir)


def main():
    parse_data()
    dump_data()
    visualize_data()

if __name__=="__main__":
    main()


