import sys
import subprocess
import os

def log(msg):
    with open("_log", 'a') as f:
        f.write(msg + '\n')

"""
Prints an error message and exits
"""
def error(msg):
    sys.stderr.write(msg)
    sys.exit(2)

"""
Reads from stdin
"""
def stdin():
    try:
        content = sys.stdin.read()
        return content
    except Exception as e:
        error("Error reading from stdin: %s" % str(e))

"""
prints to stdout
"""
def stdout(msg):
    sys.stdout.write(msg)
    sys.stdout.flush()

"""
prints to stderr
"""
def stderr(msg):
    sys.stderr.write(msg)
    sys.stderr.flush()

"""
Writes content to a file
"""
def write(file, content):
    try:
        with open(file, 'w') as f:
            f.write(content)
    except Exception as e:
        error("Error writing to file: %s" % str(e))

