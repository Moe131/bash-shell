"""
Creates output on both stderr and stdout. If an argument is supplied, then
it will use that, if no argument is supplied then it will use the contents
of stdin. If there is no stdin then the output will not contain any
additional content

Usage:
./io_combined [-c content]
./io_combined -c "hello"
    STDERR: hello
    STDOUT: hello
echo "hello" | ./io_combined 
    STDERR: hello
    STDOUT: hello

Outputs:
On Success:
    STDERR: <content>
    STDOUT: <content>
On Failure:
    appropriate error
"""

import argparse
import sys
from common import *

def main():
    parser = argparse.ArgumentParser(description="Produces output on stderr and stdout")
    parser.add_argument('-c', '--content', type=str, help='the content to print to the streams')
    parser.add_argument('-t', '--tid', help="Test ID. For use with check_zombies/kill/num_procs")

    args = parser.parse_args()
    content = ""
    if args.content is None:
        content = stdin()
    else:
        content = args.content + '\n'
    
    stderr(f'STDERR: {content}')
    stdout(f'STDOUT: {content}')
    
try:
    main()
except Exception as e:
    error("Error occurred running io_combined: " % str(e))