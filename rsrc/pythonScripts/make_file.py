"""
Creates a file in the current working directory. If the filename is specified then it
uses that filename, otherwise, it will call it stdin.txt
If content is specified, then it will fill the file with that, otherwise it will attempt
to read from stdin. If neither is given then the file will be empty

Usage:
./make_file -f filename [-c content]
echo "hello!" | ./make_file -f stdout.txt
./make_file -c "content goes here" -f created_file
./make_file -f empty_file

Outputs:
Nothing on success, appropriate error on failure
"""

import argparse
import sys
from common import *

def main():
    parser = argparse.ArgumentParser(description="Creates a file in the current working directory")
    parser.add_argument('-f', '--file', type=str, help="the name of the file to create")
    parser.add_argument('-c', '--content', type=str, help='the content to place in the file')
    parser.add_argument('-t', '--tid', help="Test ID. For use with check_zombies/kill/num_procs")


    args = parser.parse_args()
    if args.file is None:
        args.file = 'stdin.txt'

    content = ""
    if args.content is not None:
        # if content is specified, use this as the content for the file
        content = args.content + '\n'
    else:
        # if content is not specified, read from stdin
        content = stdin()
        
    write(args.file, content)

try:
    main()
except Exception as e:
    error("Error running makefile: %s" % str(e))