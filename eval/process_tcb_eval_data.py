from __future__ import division
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
TCB_SIZE_PERCENT_KEY="TCB%"
ARGS_PASSED_KEY="args_passed"

# programs the evaluation was ran for
programs = []
# coverage percentage - corresponds to subdirectories in program directory
coverages = []
#optimizations - subdirectories in each coverage dir
optimizations = []
# map {program : {coverage : {opt : tcb}}}
# is average tcb over all runs
partition_tcb = {}
partition_tcb_percent = {}
partition_functions_percent = {}
partition_coverage_functions_num = {}

partition_coverage_tcbs = {}
partition_coverage_tcbs_percent = {}
partition_coverage_functions_percent = {}
partition_coverage_functions_num = {}
# program : {coverage : avg_improvement}
ilp_improvement = {}
ilp_improvement_percent = {}
ilp_function_improvement_percent = {}


def get_bitcode_name_from_path(dir_name):
    return os.path.basename(os.path.normpath(dir_name))

def get_immediate_subdirectories(a_dir):
    return [name for name in os.listdir(a_dir)
            if os.path.isdir(os.path.join(a_dir, name))]

def average(numbers):
    import numpy as np
    return round(np.average(numbers), 1)

def median(numbers):
    from numpy import median
    return round(median(numbers), 1)

def std_deviation(numbers):
    import numpy as np
    return round(np.std(numbers), 1)

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
    y_ticks = prepare_yticks(tcb_for_program)
    #y_ticks = [100, 300, 500, 700, 900]
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


def dump_tex_tables():
    from tabulate import tabulate
    print (partition_coverage_tcbs)
    tabulate.LATEX_ESCAPE_RULES={}
    table_headers = ["", "10%", "25%", "35%", "50%", "Expert knowledge"]
    ilp_coverage_table_data = []
    ilp_coverage_percent_table_data = []

    kl_coverage_table_data = []
    kl_coverage_percent_table_data = []

    search_based_coverage_table_data = []
    search_based_coverage_percent_table_data = []

    ilp_mean = []
    ilp_median = []
    ilp_std = []
    ilp_percent_mean = []
    ilp_percent_median = []
    ilp_percent_std = []
    ilp_function_mean = []
    ilp_function_median = []
    ilp_function_std = []

    kl_mean = []
    kl_median = []
    kl_std = []
    kl_percent_mean = []
    kl_percent_median = []
    kl_percent_std = []
    kl_function_mean = []
    kl_function_median = []
    kl_function_std = []

    search_based_mean = []
    search_based_median = []
    search_based_std = []
    search_based_percent_mean = []
    search_based_percent_median = []
    search_based_percent_std = []
    search_based_function_mean = []
    search_based_function_median = []
    search_based_function_std = []

    ilp_mean.append('ilp mean')
    ilp_median.append('ilp median')
    ilp_std.append('ilp std. dev.')
    ilp_percent_mean.append('ilp mean%')
    ilp_percent_median.append('ilp median%')
    ilp_percent_std.append('ilp std. dev.%')
    ilp_function_mean.append('ilp functions mean')
    ilp_function_median.append('ilp functions median')
    ilp_function_std.append('ilp functions std. dev.')

    kl_mean.append('kl mean')
    kl_median.append('kl median')
    kl_std.append('kl std. dev.')
    kl_percent_mean.append('kl mean%')
    kl_percent_median.append('kl median%')
    kl_percent_std.append('kl std. dev.%')
    kl_function_mean.append('kl functions mean')
    kl_function_median.append('kl functions median')
    kl_function_std.append('kl functions std. dev.')

    search_based_mean.append('search based mean')
    search_based_median.append('search based median')
    search_based_std.append('search based std. dev.')
    search_based_percent_mean.append('search based mean%')
    search_based_percent_median.append('search based median%')
    search_based_percent_std.append('search based std. dev.%')
    search_based_function_mean.append('search based functions% mean')
    search_based_function_median.append('search based functions% median')
    search_based_function_std.append('search based functions% std. dev.')

    for cov_tcb in partition_coverage_tcbs:
        for opt_dir in partition_coverage_tcbs[cov_tcb]:
            mean = average(partition_coverage_tcbs[cov_tcb][opt_dir])
            med = median(partition_coverage_tcbs[cov_tcb][opt_dir])
            std_dev = std_deviation(partition_coverage_tcbs[cov_tcb][opt_dir])
            mean_percent = average(partition_coverage_tcbs_percent[cov_tcb][opt_dir])
            median_percent = median(partition_coverage_tcbs_percent[cov_tcb][opt_dir])
            std_dev_percent = std_deviation(partition_coverage_tcbs_percent[cov_tcb][opt_dir])
            function_mean = average(partition_coverage_functions_percent[cov_tcb][opt_dir]);
            function_median = median(partition_coverage_functions_percent[cov_tcb][opt_dir]);
            function_std_dev = std_deviation(partition_coverage_functions_percent[cov_tcb][opt_dir]);
            if opt_dir == 'ilp':
                ilp_mean.append(mean);
                ilp_median.append(med);
                ilp_std.append(std_dev);
                ilp_percent_mean.append(mean_percent);
                ilp_percent_median.append(median_percent);
                ilp_percent_std.append(std_dev_percent);
                ilp_function_mean.append(function_mean);
                ilp_function_median.append(function_median);
                ilp_function_std.append(function_std_dev);
            if opt_dir == 'search-based':
                search_based_mean.append(mean);
                search_based_median.append(med);
                search_based_std.append(std_dev);
                search_based_percent_mean.append(mean_percent);
                search_based_percent_median.append(median_percent);
                search_based_percent_std.append(std_dev_percent);
                search_based_function_mean.append(function_mean);
                search_based_function_median.append(function_median);
                search_based_function_std.append(function_std_dev);
            if opt_dir == 'kl':
                kl_mean.append(mean);
                kl_median.append(med);
                kl_std.append(std_dev);
                kl_percent_mean.append(mean_percent);
                kl_percent_median.append(median_percent);
                kl_percent_std.append(std_dev_percent);
                kl_function_mean.append(function_mean);
                kl_function_median.append(function_median);
                kl_function_std.append(function_std_dev);

    ilp_coverage_table_data.append(ilp_mean)
    ilp_coverage_table_data.append(ilp_median)
    ilp_coverage_table_data.append(ilp_std)
    ilp_coverage_table_data.append(ilp_percent_mean)
    ilp_coverage_table_data.append(ilp_percent_median)
    ilp_coverage_table_data.append(ilp_percent_std)
    ilp_coverage_table_data.append(ilp_function_mean)
    ilp_coverage_table_data.append(ilp_function_median)
    ilp_coverage_table_data.append(ilp_function_std)

    kl_coverage_table_data.append(kl_mean)
    kl_coverage_table_data.append(kl_median)
    kl_coverage_table_data.append(kl_std)
    kl_coverage_table_data.append(kl_percent_mean)
    kl_coverage_table_data.append(kl_percent_median)
    kl_coverage_table_data.append(kl_percent_std)
    kl_coverage_table_data.append(kl_function_mean)
    kl_coverage_table_data.append(kl_function_median)
    kl_coverage_table_data.append(kl_function_std)

    search_based_coverage_table_data.append(search_based_mean)
    search_based_coverage_table_data.append(search_based_median)
    search_based_coverage_table_data.append(search_based_std)
    search_based_coverage_table_data.append(search_based_percent_mean)
    search_based_coverage_table_data.append(search_based_percent_median)
    search_based_coverage_table_data.append(search_based_percent_std)
    search_based_coverage_table_data.append(search_based_function_mean)
    search_based_coverage_table_data.append(search_based_function_median)
    search_based_coverage_table_data.append(search_based_function_std)

    ilp_table = tabulate(ilp_coverage_table_data, headers=table_headers, tablefmt="latex")
    kl_table = tabulate(kl_coverage_table_data, headers=table_headers, tablefmt="latex")
    search_based_table = tabulate(search_based_coverage_table_data, headers=table_headers, tablefmt="latex")
    table_file_name = "tcb_eval_tables.tex"

    #table_file = os.path.join(TEX_OUT_FOLDER,"paper_tables.tex")
    table_file = os.path.join(TEX_OUT_FOLDER, table_file_name)
    with open(table_file,'wb') as tablefile:
        tablefile.write(ilp_table)
        tablefile.write(kl_table)
        tablefile.write(search_based_table)

def dump_data():
    cov_tcb_mean = {}
    cov_tcb_median = {}
    cov_tcb_std = {}
    cov_tcb_percent_mean = {}
    cov_tcb_percent_median = {}
    cov_tcb_percent_std = {}
    cov_tcb_functions_percent_mean = {}
    cov_tcb_functions_percent_median = {}
    cov_tcb_functions_percent_std = {}

    for cov_tcb in partition_coverage_tcbs:
        cov_tcb_mean[cov_tcb] = {}
        cov_tcb_median[cov_tcb] = {}
        cov_tcb_std[cov_tcb] = {}
        cov_tcb_percent_mean[cov_tcb] = {}
        cov_tcb_percent_median[cov_tcb] = {}
        cov_tcb_percent_std[cov_tcb] = {}
        cov_tcb_functions_percent_mean[cov_tcb] = {}
        cov_tcb_functions_percent_median[cov_tcb] = {}
        cov_tcb_functions_percent_std[cov_tcb] = {}
        for opt_dir in partition_coverage_tcbs[cov_tcb]:
            cov_tcb_mean[cov_tcb][opt_dir] = average(partition_coverage_tcbs[cov_tcb][opt_dir])
            cov_tcb_median[cov_tcb][opt_dir] = median(partition_coverage_tcbs[cov_tcb][opt_dir])
            cov_tcb_std[cov_tcb][opt_dir] = std_deviation(partition_coverage_tcbs[cov_tcb][opt_dir])
            cov_tcb_percent_mean[cov_tcb][opt_dir] = average(partition_coverage_tcbs_percent[cov_tcb][opt_dir])
            cov_tcb_percent_median[cov_tcb][opt_dir] = median(partition_coverage_tcbs_percent[cov_tcb][opt_dir])
            cov_tcb_percent_std[cov_tcb][opt_dir] = std_deviation(partition_coverage_tcbs_percent[cov_tcb][opt_dir])
            cov_tcb_functions_percent_mean[cov_tcb][opt_dir] = average(partition_coverage_functions_percent[cov_tcb][opt_dir])
            cov_tcb_functions_percent_median[cov_tcb][opt_dir] = median(partition_coverage_functions_percent[cov_tcb][opt_dir])
            cov_tcb_functions_percent_std[cov_tcb][opt_dir] = std_deviation(partition_coverage_functions_percent[cov_tcb][opt_dir])

    coverage_file = "secure_tcb_eval.json"
    with open(coverage_file, 'a') as f :
        f.write(json.dumps({"programs tcbs" : partition_tcb}, indent=4))
        f.write(json.dumps({"programs tcbs%" : partition_tcb_percent}, indent=4))
        f.write(json.dumps({"programs tcbs functions%" : partition_functions_percent}, indent=4))
        f.write(json.dumps({"coverage mean" : cov_tcb_mean}, indent = 4))
        f.write(json.dumps({"coverage median" : cov_tcb_median}, indent = 4))
        f.write(json.dumps({"coverage std" : cov_tcb_std}, indent = 4))
        f.write(json.dumps({"coverage mean%" : cov_tcb_percent_mean}, indent = 4))
        f.write(json.dumps({"coverage median%" : cov_tcb_percent_median}, indent = 4))
        f.write(json.dumps({"coverage std%" : cov_tcb_percent_std}, indent = 4))
        f.write(json.dumps({"function coverage mean%" : cov_tcb_functions_percent_mean}, indent = 4))
        f.write(json.dumps({"function coverage median%" : cov_tcb_functions_percent_median}, indent = 4))
        f.write(json.dumps({"function coverage std%" : cov_tcb_functions_percent_std}, indent = 4))
        f.write(json.dumps({"ilp improvement" : ilp_improvement} , indent = 4))
        f.write(json.dumps({"ilp improvement%" : ilp_improvement_percent} , indent = 4))
        f.write(json.dumps({"ilp function improvement%" : ilp_improvement_percent} , indent = 4))

def parse_functions_data_for_program_coverage_optimization(program, coverage, repeat_dir, opt_dir):
    stat_file = os.path.join(STAT_DIR, program, coverage, repeat_dir, opt_dir, STAT_FILE)
    #if program not in partition_tcb:
    #    partition_tcb[program] = {}
    #if coverage not in partition_tcb[program]:
    #    partition_tcb[program][coverage] = {}
    #if opt not in partition_tcb[program][coverage]:
    #    partition_tcb[program][coverage][opt] = 0
    if not os.path.isfile(stat_file):
        return
    stats = json.load(open(stat_file))
    return stats["partition"]["secure_partition"][PARTITION_PERCENT_KEY]

def parse_data_for_program_coverage_optimization_percent(program, coverage, repeat, opt):
    stat_file = os.path.join(STAT_DIR, program, coverage, repeat, opt, STAT_FILE)
    #if program not in partition_tcb:
    #    partition_tcb[program] = {}
    #if coverage not in partition_tcb[program]:
    #    partition_tcb[program][coverage] = {}
    #if opt not in partition_tcb[program][coverage]:
    #    partition_tcb[program][coverage][opt] = 0
    if not os.path.isfile(stat_file):
        return
    stats = json.load(open(stat_file))
    return stats["partition"]["secure_partition"][TCB_SIZE_PERCENT_KEY]


def parse_data_for_program_coverage_optimization(program, coverage, repeat, opt):
    stat_file = os.path.join(STAT_DIR, program, coverage, repeat, opt, STAT_FILE)
    #if program not in partition_tcb:
    #    partition_tcb[program] = {}
    #if coverage not in partition_tcb[program]:
    #    partition_tcb[program][coverage] = {}
    #if opt not in partition_tcb[program][coverage]:
    #    partition_tcb[program][coverage][opt] = 0
    if not os.path.isfile(stat_file):
        return
    stats = json.load(open(stat_file))
    return stats["partition"]["secure_partition"][TCB_SIZE_KEY]

def parse_data_for_manual_coverage(program):
    coverage = "expert-knowledge";
    opt_dirs = get_immediate_subdirectories(os.path.join(STAT_DIR, program, coverage));
    for opt_dir in opt_dirs:
        if opt_dir not in optimizations:
            optimizations.append(opt_dir)
        if opt_dir not in partition_coverage_tcbs:
            partition_coverage_tcbs[coverage][opt_dir] = []
            partition_coverage_tcbs_percent[coverage][opt_dir] = []
            partition_coverage_functions_percent[coverage][opt_dir] = []
        partition_tcb[program][coverage][opt_dir] = 0
        partition_tcb_percent[program][coverage][opt_dir] = 0
        partition_functions_percent[program][coverage][opt_dir] = 0
        tcb_size = parse_data_for_program_coverage_optimization(program, coverage, "", opt_dir)
        tcb_size_percent = parse_data_for_program_coverage_optimization_percent(program, coverage, "", opt_dir)
        functions_size_percent = parse_functions_data_for_program_coverage_optimization(program, coverage, "", opt_dir)
        partition_tcb[program][coverage][opt_dir] = tcb_size
        partition_tcb_percent[program][coverage][opt_dir] = tcb_size_percent
        partition_functions_percent[program][coverage][opt_dir] = tcb_size_percent
        #if opt_dir == "ilp":
        partition_coverage_tcbs[coverage][opt_dir].append(tcb_size)
        partition_coverage_tcbs_percent[coverage][opt_dir].append(tcb_size_percent)
        partition_coverage_functions_percent[coverage][opt_dir].append(functions_size_percent)

def parse_data_for_program_coverage(program, coverage):
    if (coverage == "expert-knowledge"):
        parse_data_for_manual_coverage(program)
        return

    repeat_dirs = get_immediate_subdirectories(os.path.join(STAT_DIR, program, coverage))
    repeat_opt_tcbs = {}
    repeat_opt_tcbs_percent = {}
    repeat_opt_function_percent = {}
    for repeat_dir in repeat_dirs:
        opt_dirs = get_immediate_subdirectories(os.path.join(STAT_DIR, program, coverage, repeat_dir))
        for opt_dir in opt_dirs:
            if opt_dir not in partition_coverage_tcbs[coverage]:
                partition_coverage_tcbs[coverage][opt_dir] = []
                partition_coverage_tcbs_percent[coverage][opt_dir] = []
                partition_coverage_functions_percent[coverage][opt_dir] = []
            if opt_dir not in optimizations:
                optimizations.append(opt_dir)
            if opt_dir not in repeat_opt_tcbs:
                 repeat_opt_tcbs[opt_dir] = []
                 repeat_opt_tcbs_percent[opt_dir] = []
                 repeat_opt_function_percent[opt_dir] = []
            tcb_size = parse_data_for_program_coverage_optimization(program, coverage, repeat_dir, opt_dir)
            tcb_size_percent = parse_data_for_program_coverage_optimization_percent(program, coverage, repeat_dir, opt_dir)
            functions_size_percent = parse_data_for_program_coverage_optimization_percent(program, coverage, repeat_dir, opt_dir)
            functions_size_percent = parse_functions_data_for_program_coverage_optimization(program, coverage, repeat_dir, opt_dir)
            repeat_opt_tcbs[opt_dir].append(tcb_size)
            repeat_opt_tcbs_percent[opt_dir].append(tcb_size_percent)
            repeat_opt_function_percent[opt_dir].append(functions_size_percent)
            #if opt_dir == "ilp":
            partition_coverage_tcbs[coverage][opt_dir].append(tcb_size)
            partition_coverage_tcbs_percent[coverage][opt_dir].append(tcb_size_percent)
            partition_coverage_functions_percent[coverage][opt_dir].append(functions_size_percent)

    for opt in repeat_opt_tcbs:
        partition_tcb[program][coverage][opt] = sum(repeat_opt_tcbs[opt])/len(repeat_opt_tcbs[opt])
        partition_tcb_percent[program][coverage][opt] = sum(repeat_opt_tcbs_percent[opt])/len(repeat_opt_tcbs_percent[opt])
        partition_functions_percent[program][coverage][opt] = sum(repeat_opt_tcbs_percent[opt])/len(repeat_opt_tcbs_percent[opt])

def parse_data_for_program(program):
    cov_dirs = get_immediate_subdirectories(os.path.join(STAT_DIR, program))
    for cov_dir in cov_dirs:
        if cov_dir not in coverages:
            coverages.append(cov_dir)
        partition_tcb[program][cov_dir] = {}
        partition_tcb_percent[program][cov_dir] = {}
        partition_functions_percent[program][cov_dir] = {}
        if cov_dir not in partition_coverage_tcbs:
            partition_coverage_tcbs[cov_dir] = {}
            partition_coverage_tcbs_percent[cov_dir] = {}
            partition_coverage_functions_percent[cov_dir] = {}
        parse_data_for_program_coverage(program, cov_dir)
        ilp_improvement[program][cov_dir] = partition_tcb[program][cov_dir]["search-based"]/partition_tcb[program][cov_dir]["ilp"]
        ilp_improvement_percent[program][cov_dir] = partition_tcb_percent[program][cov_dir]["search-based"]/partition_tcb_percent[program][cov_dir]["ilp"]
        ilp_function_improvement_percent[program][cov_dir] = partition_functions_percent[program][cov_dir]["search-based"]/partition_functions_percent[program][cov_dir]["ilp"]

def parse_data():
    rootDir = STAT_DIR
    directories = get_immediate_subdirectories(rootDir)
    print ("Programs evaluation was ran for ")
    print (directories)
    for program_dir in directories:
        if program_dir not in programs:
            programs.append(program_dir)
        partition_tcb[program_dir] = {}
        partition_tcb_percent[program_dir] = {}
        partition_functions_percent[program_dir] = {}
        ilp_improvement[program_dir] = {}
        ilp_improvement_percent[program_dir] = {}
        ilp_function_improvement_percent[program_dir] = {}
        parse_data_for_program(program_dir)


def main():
    parse_data()
    dump_data()
    dump_tex_tables()
    visualize_data()

if __name__=="__main__":
    main()


