"""
Runs infinitely

Usage:
./infinite

Outputs:
Nothing
"""
import argparse
import sys
import time
from common import *

def main():
    parser = argparse.ArgumentParser(description="Runs indefinitely until killed. Note: this will become an orphan process unless you manually kill it.")
    parser.add_argument('time', help='number of seconds to sleep for', type=int)
    parser.add_argument('-t', '--tid', help="Test ID. For use with num_procs.py and check_zombies.py")

    while True:
        time.sleep(1)

try:
    main()
except Exception as e:
    error("Error running sleep: %s" % str(e))