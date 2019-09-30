#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Sep 20 10:10:45 2019

@author: Macrobull
"""

from __future__ import division

import re


def level_filter(
        record_stream:'Iterable[record]', level:int)->'Iterable[record]':
    """filter record by level"""

    level = int(level)
    return filter(lambda record: record.level >= level, record_stream)


def thread_filter(
        record_stream:'Iterable[record]',
        thread:'Union[int, Sequence[int]]')->'Iterable[record]':
    """filter record by thread_id"""

    thread_ids = thread if isinstance(thread, (list, tuple, set)) else (thread, )
    thread_ids = list(map(int, thread_ids))
    return filter(lambda record: record.thread_id in thread_ids, record_stream)


def filename_filter(
        record_stream:'Iterable[record]', filename_pattern:str)->'Iterable[record]':
    """filter record by filename pattern"""

    regex = re.compile(filename_pattern)
    return filter(lambda record: regex.fullmatch(record.filename), record_stream)
