"""
Creates output on stdout. If an argument is supplied, then
it will use that, if no argument is supplied then it will use the contents
of stdin. If there is no stdin then the output will not contain any
additional content

Usage:
./io_stdout [-c content]
./io_stdout -c "hello"
    STDERR: hello
echo "hello" | ./io_stdout 
    STDERR: hello

Outputs:
On Success:
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
    
    stdout(content)
    
try:
    main()
except Exception as e:
    error("Error occurred running io_stdout: " % str(e))