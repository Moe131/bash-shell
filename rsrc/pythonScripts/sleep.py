"""
Sleeps for the given integer (in seconds)

Usage:
./sleep [time]
./sleep 1

Outputs:
Nothing
"""
import argparse
import sys
import time
from common import *

def main():
    parser = argparse.ArgumentParser(description="Sleeps for the specified number of seconds")
    parser.add_argument('time', help='number of seconds to sleep for', type=int)
    parser.add_argument('-t', '--tid', help="Test ID. For use with check_zombies/kill/num_procs")


    args = parser.parse_args()
    if args.time is None:
        raise Exception('time is a required argument')

    time.sleep(args.time)

try:
    main()
except Exception as e:
    error("Error running sleep: %s" % str(e))