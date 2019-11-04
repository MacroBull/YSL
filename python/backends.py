#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun Sep 15 22:09:46 2019

@author: Macrobull
"""

from __future__ import absolute_import, division, unicode_literals

from subprocess import Popen, PIPE


def set_non_block(io:'io.IOBase')->'Any':
    """set io/file/fd non-blocking"""

    import fcntl, os

    fd = io.fileno()
    fl = fcntl.fcntl(fd, fcntl.F_GETFL)
    return fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)


def launch(args:'Union[Sequence[str], str]',
           **kwargs)->Popen:
    """launch process"""

    if isinstance(args, str):
        proc = Popen(args, stdout=PIPE, shell=True, **kwargs)
    else:
        proc = Popen(list(args), stdout=PIPE, shell=False, **kwargs)
    return proc


def tailf(filename:str,
          **kwargs)->Popen:
    """tail follow"""

    proc = Popen(['tail', '-Fn0', filename], stdout=PIPE, **kwargs)
    return proc


def tailc(filename:str,
          **kwargs)->Popen:
    """tail cat and follow"""

    proc = Popen(['tail', '-Fc+0', filename], stdout=PIPE, **kwargs)
    return proc


def ssh_tailc(address:str, filename:str,
              **kwargs)->Popen:
    """tail cat and follow via ssh"""

    proc = Popen(f'ssh {address} "tail -Fc+0 {filename}"',
                 stdout=PIPE, shell=True, **kwargs)
    return proc


if __name__ == '__main__':
    proc = tailc('/tmp/test.log')
    for line in proc.stdout:
        print(line)
