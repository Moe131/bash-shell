"""
Creates output on stderr. If an argument is supplied, then
it will use that, if no argument is supplied then it will use the contents
of stdin. If there is no stdin then the output will not contain any
additional content

Usage:
./io_stderr [-c content]
./io_stderr -c "hello"
    STDERR: hello
echo "hello" | ./io_stderr 
    STDERR: hello

Outputs:
On Success:
    STDERR: <content>
On Failure:
    appropriate error
"""

import argparse
import sys
from common import *

def main():
    parser = argparse.ArgumentParser(description="Produces output on stderr and stderr")
    parser.add_argument('-c', '--content', type=str, help='the content to print to the streams')
    parser.add_argument('-t', '--tid', help="Test ID. For use with check_zombies/kill/num_procs")

    args = parser.parse_args()
    content = ""
    if args.content is None:
        content = stdin()
    else:
        content = args.content + '\n'
    
    stderr(content)
    
try:
    main()
except Exception as e:
    error("Error occurred running io_stderr: " % str(e))