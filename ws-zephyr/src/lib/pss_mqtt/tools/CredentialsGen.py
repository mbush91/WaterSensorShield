#!/usr/bin/env python3
from jinja2 import Template
from datetime import datetime
import argparse
import os
import glob
import sys

def parse_args(argv:list=None):
    """Parses command line arguments and returns them as a namespace.

    Args:
        argv (list, optional): List of command to be parse as argument. Defaults to None.

    Returns:
        Namespace: Namespace containing specified arguments.
    """
    parser = argparse.ArgumentParser(description='Generates C configurations for Scheduler component.')

    parser.add_argument('-i','--cert-folder', action='store',default="../cert", help='Folder Containing Thingstream.io Certs')
    parser.add_argument('-o','--output-folder', action='store',default="../gen", help='folder to store the generated files.')
    parser.add_argument('-t','--templates', action='store',default="./templates", help="folder containing jinja templates.")
    parser.add_argument('--no-rel',action="store_false",help="Argument paths are relative to this script unless this is set.")
    parser.add_argument('-c','--check',action="store_true",help="Checks if generation output matches the current files. Exits with error if they do not match.")

    args = parser.parse_args(argv)

    if args.no_rel :
        cur = os.path.dirname(__file__)

        args.cert_folder = os.path.join(cur,args.cert_folder)
        assert os.path.exists(args.cert_folder)

        args.output_folder = os.path.join(cur,args.output_folder)
        if not os.path.exists(args.output_folder) :
            os.mkdir(args.output_folder)

        args.templates = os.path.join(cur,args.templates)
        assert os.path.exists(args.templates)

    return args

class NoCertsException(Exception):
    "Raised when the input value is less than 18"
    pass

def load_config(args) -> dict:
    """Reads the CSV map file and converts each column to a dictionary.

    Args:
        csv_file (str): Path to csv file.

    Returns:
        dict: Dictionary of each column in the csv representing the task and it runnables.
    """

    certs = glob.glob(args.cert_folder+"/*")

    if len(certs) == 0:
        raise NoCertsException("no certs")


    info = { 'certs' : {} }
    cid_fnd = False

    for c in certs :
        if (not cid_fnd) and ('.crt' in c) :
            try :
                cid = os.path.basename(c).split('-')[0]
                cid_fnd = True
                info['CID'] = cid
            except :
                pass
        with open(c,'r') as f :
            ext = os.path.basename(c).split('.')[-1]
            if ext == 'pem' and 'Root' in os.path.basename(c) :
                crt_type = 'root'
            elif ext == 'pem' :
                crt_type = 'key'
            else :
                crt_type = 'crt'

            if crt_type in info['certs'].keys() :
                raise Exception("Invalid Certs!")

            info['certs'][crt_type] = {}
            lines = f.readlines()
            info['certs'][crt_type]['lines'] = [ l.replace('\n','') for l in lines]
            info['certs'][crt_type]['name'] = os.path.basename(c)

    return info

def run_clang_format(filename) :
    """Runs clang-format on the given file.

    Args:
        filename (str): Path to file to run clang-format on.
    """
    os.system(f"clang-format -i {filename}")

def generate_files(info:dict, args) :
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

        now = datetime.now()
        datetime_string = now.strftime("%Y-%m-%d %H:%M:%S")

        content = j_temp.render(info=info,time=datetime_string, cid_len=len(info['CID']))

        if content[-1] != '\n' :
            content = content + '\n'

        if args.check :
            with open(output,'r') as d :
                dest = d.read()
            if content != dest :
                print("Generated content for %s does not match! Did you modify the generated code or forget to re-generate??"%fname,
                    file=sys.stderr)
                sys.exit(1)
        else :
            with open(output,'w') as out :
                out.write(content)

            run_clang_format(output)


def main(args:list=None) :
    """Main function to run script.

    Args:
        argv (list, optional): List of command to be parse as argument. Defaults to None.
    """

    args = parse_args(args)

    try:
        info = load_config(args)
    except NoCertsException as e:
        print("no certs, doing nothing")
        sys.exit(0);


    generate_files(info,args)

if __name__ == '__main__':
    main()
