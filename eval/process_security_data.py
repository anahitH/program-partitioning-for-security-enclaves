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
SECURITY_RELATED_PERCENT="security_related%"
SECURITY_RELATED_FUNCTIONS="security_related_functions"

# programs the evaluation was ran for
programs = []
# coverage percentage - corresponds to subdirectories in program directory
coverages = []
#optimizations - subdirectories in each coverage dir
optimizations = []

# map {coverage : {opt : [security_related_%]}}
security_related_percent = {}

# map {coverage : {opt : [security_related level]}}
security_related = {}

def get_bitcode_name_from_path(dir_name):
    return os.path.basename(os.path.normpath(dir_name))

def get_immediate_subdirectories(a_dir):
    return [name for name in os.listdir(a_dir)
            if os.path.isdir(os.path.join(a_dir, name))]

def average(numbers):
    import numpy as np
    if not numbers:
        return 0
    return round(np.average(numbers), 1)

def median(numbers):
    from numpy import median
    if not numbers:
        return 0
    return round(median(numbers), 1)

def std_deviation(numbers):
    import numpy as np
    if not numbers:
        return 0
    return round(np.std(numbers), 1)

def parse_data_for_security_related_percent(program, coverage, repeat, opt):
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
    return stats["partition"]["secure_partition"][SECURITY_RELATED_PERCENT]

def parse_data_for_security_related_level(program, coverage, repeat, opt):
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
    security_related_level_data = stats["partition"]["secure_partition"][SECURITY_RELATED_FUNCTIONS]
    security_related_level = []
    for data in security_related_level_data:
        security_related_level.append((int)(data.split(" ")[-1]))

    return security_related_level

def parse_data_for_manual_coverage(program):
    coverage = "expert-knowledge";
    opt_dirs = get_immediate_subdirectories(os.path.join(STAT_DIR, program, coverage));
    for opt_dir in opt_dirs:
        if opt_dir not in optimizations:
            optimizations.append(opt_dir)
        if opt_dir not in security_related_percent:
            security_related_percent[coverage][opt_dir] = []
            security_related[coverage][opt_dir] = []

        sec_rel_percent = parse_data_for_security_related_percent(program, coverage, "", opt_dir)
        sec_rel_level = parse_data_for_security_related_level(program, coverage, "", opt_dir)
        security_related_percent[coverage][opt_dir].append(sec_rel_percent)
        security_related[coverage][opt_dir].extend(sec_rel_level)

def parse_data_for_program_coverage(program, coverage):
    if (coverage == "expert-knowledge"):
        parse_data_for_manual_coverage(program)
        return

    repeat_dirs = get_immediate_subdirectories(os.path.join(STAT_DIR, program, coverage))
    for repeat_dir in repeat_dirs:
        opt_dirs = get_immediate_subdirectories(os.path.join(STAT_DIR, program, coverage, repeat_dir))
        for opt_dir in opt_dirs:
            if opt_dir not in security_related[coverage]:
                security_related[coverage][opt_dir] = []

            if opt_dir not in security_related_percent[coverage]:
                security_related_percent[coverage][opt_dir] = []

            sec_rel_percent = parse_data_for_security_related_percent(program, coverage, repeat_dir, opt_dir)
            sec_rel_level = parse_data_for_security_related_level(program, coverage, repeat_dir, opt_dir)
            security_related[coverage][opt_dir].extend(sec_rel_level)
            security_related_percent[coverage][opt_dir].append(sec_rel_percent)

def parse_data_for_program(program):
    cov_dirs = get_immediate_subdirectories(os.path.join(STAT_DIR, program))
    for cov_dir in cov_dirs:
        if cov_dir not in coverages:
            coverages.append(cov_dir)
        if cov_dir not in security_related:
            security_related[cov_dir] = {}
        if cov_dir not in security_related_percent:
            security_related_percent[cov_dir] = {}
        parse_data_for_program_coverage(program, cov_dir)

def parse_data():
    rootDir = STAT_DIR
    directories = get_immediate_subdirectories(rootDir)
    print ("Programs evaluation was ran for ")
    print (directories)
    for program_dir in directories:
        if program_dir not in programs:
            programs.append(program_dir)
        parse_data_for_program(program_dir)

def dump_tex_tables():
    from tabulate import tabulate
    tabulate.LATEX_ESCAPE_RULES={}
    table_headers = ["", "10%", "25%", "35%", "50%", "Expert knowledge"]
    ilp_security_level_percent_table_data = []
    ilp_security_level_table_data = []
    no_opt_security_level_table_data = []


    ilp_mean = []
    ilp_median = []
    ilp_std = []
    ilp_percent_mean = []
    ilp_percent_median = []
    ilp_percent_std = []

    no_opt_mean = []
    no_opt_median = []
    no_opt_std = []

    ilp_mean.append('ilp mean')
    ilp_median.append('ilp median')
    ilp_std.append('ilp std. dev.')
    ilp_percent_mean.append('ilp mean%')
    ilp_percent_median.append('ilp median%')

    no_opt_mean.append('ilp mean')
    no_opt_median.append('ilp median')
    no_opt_std.append('ilp std. dev.')

    opt_dirs = ['ilp', 'no-opt']
    for cov_tcb in coverages:
        for opt in opt_dirs:
            mean = average(security_related[cov_tcb][opt])
            med = median(security_related[cov_tcb][opt])
            std_dev = std_deviation(security_related[cov_tcb][opt])
            mean_percent = average(security_related_percent[cov_tcb][opt])
            median_percent = median(security_related_percent[cov_tcb][opt])
            std_dev_percent = std_deviation(security_related_percent[cov_tcb][opt])
            if opt == 'ilp':
                ilp_mean.append(mean);
                ilp_median.append(med);
                ilp_std.append(std_dev);
                ilp_percent_mean.append(mean_percent);
                ilp_percent_median.append(median_percent);
                ilp_percent_std.append(std_dev_percent)
            if opt == 'no-opt':
                no_opt_mean.append(mean);
                no_opt_median.append(med);
                no_opt_std.append(std_dev);

    ilp_security_level_table_data.append(ilp_mean)
    ilp_security_level_table_data.append(ilp_median)
    ilp_security_level_table_data.append(ilp_std)
    ilp_security_level_percent_table_data.append(ilp_percent_mean)
    ilp_security_level_percent_table_data.append(ilp_percent_median)
    ilp_security_level_percent_table_data.append(ilp_percent_std)
    no_opt_security_level_table_data.append(no_opt_mean)
    no_opt_security_level_table_data.append(no_opt_median)
    no_opt_security_level_table_data.append(no_opt_std)

    ilp_table = tabulate(ilp_security_level_table_data, headers=table_headers, tablefmt="latex")
    ilp_percent_table = tabulate(ilp_security_level_percent_table_data, headers=table_headers, tablefmt="latex")
    no_opt_table = tabulate(no_opt_security_level_table_data, headers=table_headers, tablefmt="latex")
    table_file_name = "security_eval_tables.tex"

    #table_file = os.path.join(TEX_OUT_FOLDER,"paper_tables.tex")
    table_file = os.path.join(TEX_OUT_FOLDER, table_file_name)
    with open(table_file,'wb') as tablefile:
        tablefile.write(no_opt_table)
        tablefile.write(ilp_table)
        tablefile.write(ilp_percent_table)

def main():
    parse_data()
#    dump_data()
    dump_tex_tables()

if __name__=="__main__":
    main()


