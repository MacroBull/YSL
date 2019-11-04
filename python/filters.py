#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Sep 20 10:10:45 2019

@author: Macrobull
"""

from __future__ import absolute_import, division, unicode_literals

import re


def is_listy(obj:'Any')->bool:
    """is `obj` 'list'-like"""

    return isinstance(obj, (tuple, list, set))


def level_filter(
        record_stream:'Iterable[record]',
        level:'Union[int, Sequence[int]]')->'Iterable[record]':
    """filter record by level"""

    levels = level if is_listy(level) else (level, )
    levels = list(map(int, levels))
    return filter(lambda record: record.level in levels, record_stream)


def thread_filter(
        record_stream:'Iterable[record]',
        thread:'Union[int, Sequence[int]]')->'Iterable[record]':
    """filter record by thread_id"""

    thread_ids = thread if is_listy(thread) else (thread, )
    thread_ids = list(map(int, thread_ids))
    return filter(lambda record: record.thread_id in thread_ids, record_stream)


def filename_filter(
        record_stream:'Iterable[record]', filename_pattern:str)->'Iterable[record]':
    """filter record by filename pattern"""

    regex = re.compile(filename_pattern)
    return filter(lambda record: regex.fullmatch(record.filename), record_stream)
