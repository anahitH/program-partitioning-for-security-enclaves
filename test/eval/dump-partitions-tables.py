import os
import json
from pprint import pprint

STAT_DIR="/home/anahitik/TUM/Thesis/program-partitioning-for-security-enclaves/test/eval/partition-out"
STAT_FILE="partition_stats.json"
TEX_OUT_FOLDER='tables'
PARTITION_PERCENT_KEY="partition%"

programs = []
optimizations = []
partition_percentage = {}

def get_bitcode_name_from_path(dir_name):
    return os.path.basename(os.path.normpath(dir_name))

def get_immediate_subdirectories(a_dir):
    return [name for name in os.listdir(a_dir)
            if os.path.isdir(os.path.join(a_dir, name))]

def parse_statistics(program, stat_dir, opt_name):
    stat_file = os.path.join(STAT_DIR, program)
    if program != stat_dir:
        stat_file = os.path.join(stat_file, stat_dir, STAT_FILE)
    else:
        stat_file = os.path.join(stat_file, STAT_FILE)
    if opt_name not in optimizations:
        optimizations.append(opt_name)
    if program not in partition_percentage:
        partition_percentage[program] = {}
    if not os.path.isfile(stat_file):
        partition_percentage[program][opt_name] = "N/A"
        return
    stats = json.load(open(stat_file))
    partition_percentage[program][opt_name] = stats["program_partition"][PARTITION_PERCENT_KEY]


def parse_data():
    rootDir = STAT_DIR
    directories = get_immediate_subdirectories(rootDir)
    print (directories)
    for program_dir in directories:
        programs.append(program_dir)
        parse_statistics(program_dir, program_dir, "no-opt")
        opt_dirs = get_immediate_subdirectories(os.path.join(rootDir, program_dir))
        for opt_dir in opt_dirs:
            parse_statistics(program_dir, opt_dir, os.path.basename(opt_dir))

def dump_tables():
    from tabulate import tabulate
    tabulate.LATEX_ESCAPE_RULES={}
    table_headers = ["Program"]
    table_data = []
    for opt in optimizations:
        if opt == "static-analysis":
            table_headers.append(opt + "[Baseline]")
        else:
            table_headers.append(opt)

    for program in programs:
        line = []
        line.append(program)
        for opt in optimizations:
            line.append(partition_percentage[program][opt])
        print (line)
        table_data.append(line)

    latex_table = tabulate(table_data,headers=table_headers,tablefmt="latex")
    table_file = os.path.join(TEX_OUT_FOLDER, "partitions.tex")
    with open(table_file,'wb') as tablefile:
        tablefile.write(latex_table)

       
def main():
    parse_data()
    dump_tables()

if __name__=="__main__":
    main()

