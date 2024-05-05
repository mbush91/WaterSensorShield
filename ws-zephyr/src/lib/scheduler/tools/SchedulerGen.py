#!/usr/bin/env python3
from itertools import cycle
from jinja2 import Template
import argparse
import pandas as pd
import os
import glob
import sys
import re
import subprocess

def parse_args(argv:list=None):
    """Parses command line arguments and returns them as a namespace.

    Args:
        argv (list, optional): List of command to be parse as argument. Defaults to None.

    Returns:
        Namespace: Namespace containing specified arguments.
    """
    parser = argparse.ArgumentParser(description='Generates C configurations for Scheduler component.')

    parser.add_argument('-i','--csv', action='store',default="../cfg/scheduler_map.csv", help='CSV containing runnable map.')
    parser.add_argument('-o','--output-folder', action='store',default="../gen/", help='folder to store the generated files.')
    parser.add_argument('-t','--templates', action='store',default="./templates", help="folder containing jinja templates.")
    parser.add_argument('--no-rel',action="store_false",help="Argument paths are relative to this script unless this is set.")
    parser.add_argument('-c','--check',action="store_true",help="Checks if generation output matches the current files. Exits with error if they do not match.")

    args = parser.parse_args(argv)

    if args.no_rel :
        cur = os.path.dirname(__file__)

        args.csv = os.path.join(cur,args.csv)
        assert os.path.exists(args.csv)

        args.output_folder = os.path.join(cur,args.output_folder)
        assert os.path.exists(args.output_folder)

        args.templates = os.path.join(cur,args.templates)
        assert os.path.exists(args.templates)

    return args

def load_config(csv_file, check) -> dict:
    """Reads the CSV map file and converts each column to a dictionary.

    Args:
        csv_file (str): Path to csv file.

    Returns:
        dict: Dictionary of each column in the csv representing the task and it runnables.
    """

    with open(csv_file, 'r') as f:
        orig = f.read()
        f.seek(0)
        df = pd.read_csv(f)

    # strip all columns and cells
    df.columns = df.columns.str.strip()
    df = df.applymap(lambda x: x.strip() if isinstance(x, str) else x)
    df = df.fillna('')
    df = df.loc[:, ~df.columns.str.contains('^Unnamed')]


    new_csv = df.rename(columns=lambda x: f"{x},").to_string(formatters=[lambda x: f"{x}," for y in df.columns], index=False).replace(r' *', "")
    new_csv = re.sub(r' *,$', "", new_csv, flags=re.MULTILINE)
    new_csv += "\n"

    if check:
        if orig != new_csv:
            print("csv isn't formatted", file=sys.stderr)
            print("please run SchedulerGen.py", file=sys.stderr)
            sys.exit(2)
    else:
        with open(csv_file,'w') as out:
            out.write(new_csv)

    config = {}
    includes = []

    for i in range(1,len(df.columns)) :
        name = df.columns[i]
        data = df.loc[:,name]
        components = [ c for c in list(data[3:data.size]) if c != "" ]
        incs = [ c.split('_main')[0] for c in components if c.split('_main')[0] not in includes ]
        includes.extend(incs)
        config[name] = {
                "name":name,
                "cycleTime":(int(data[0]) if data[0] != 'T' else data[0]),
                "priority":int(data[1]),
                "stackSize":int(data[2]),
                "runnables":components,
                "includes":incs
        }

    return config

def run_clang_format(input) :
    """Runs clang-format on the given input string

    """
    conf = os.path.realpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "..", "..", "..", "..", "..", ".clang-format"))
    p = subprocess.run(["clang-format", f"--style=file:{conf}"], input=input, capture_output=True, encoding='ascii')
    return p.stdout

def generate_files(config:dict, args) :
    """Generates or checks files from Jinja2 templates using config read by load_config function.

    Args:
        config (dict): Config returned from load_config function
        args (_type_): Namespace of command-line/default arguments.
    """

    templates = glob.glob(args.templates+"/*.jinja")

    for t in templates :
        fname = os.path.basename(t).replace(".jinja",'')
        output = os.path.join(args.output_folder,fname)

        with open(t,'r') as f :
            j_temp = Template(f.read(),trim_blocks=True)

        content = j_temp.render(config=config)

        if content[-1] != '\n' :
            content = content + '\n'

        if args.check :
            with open(output,'r') as d :
                actual = d.read()

            formatted = run_clang_format(content)

            if formatted != actual :
                print(f"Generated content for {fname} does not match! Did you modify the generated code or forget to re-generate??",
                    file=sys.stderr)
                print("please run SchedulerGen.py", file=sys.stderr)
                sys.exit(2)
        else :
            with open(output,'w') as out :
                out.write(content)


def main(args:list=None) :
    """Main function to run script.

    Args:
        argv (list, optional): List of command to be parse as argument. Defaults to None.
    """

    args = parse_args(args)

    config = load_config(args.csv, args.check)

    generate_files(config,args)

if __name__ == '__main__':
    main()
