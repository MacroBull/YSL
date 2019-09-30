#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon Sep 16 13:36:20 2019

@author: Macrobull
"""

from __future__ import division

import re

from collections import namedtuple
from datetime import datetime


GLOG_HEAD_GROUP_NAME :tuple = ('level', 'date', 'thread_id', 'filename', 'line', 'msg')
GLOG_HEAD_PATTEN = r'([IWEF])(\d{4}\s+\d\d:\d\d:\d\d\.\d{6})\s+(\d+)\s+(\w+\.\w+):(\d+)\]\s'
GLOG_PERROR_PATTEN = r'C(ould not create log|OULD NOT CREATE LOG).+\n'
GLOG_HEAD_REGEX = re.compile(GLOG_HEAD_PATTEN)
GLOG_PERROR_REGEX = re.compile(GLOG_PERROR_PATTEN)

record :type = namedtuple('record', GLOG_HEAD_GROUP_NAME)


def get_msg(record_stream:"Iterable[record]")->"Iterable[str]":
    """get msg field in record"""
    
    return map(lambda rec: rec.msg, record_stream)


class GlogParser(object):
    """
    GlogParser:
        if `text_stream` given, parser is iterable within this stream
        if `hold_last` is True, the last record will not be emitted by default
    """

    LEVEL_MAPPING :'Mapping[Any, int]' = {
            0: 0, 1: 1, 2: 2, 3: 3,
            'I': 0, 'W': 1, 'E': 2, 'F': 3,
            'INFO': 0, 'WARNING': 1, 'ERROR': 2, 'FATAL': 3,
            }

    @classmethod
    def make_record(
            cls,
            *args:'Tuple[Any, ...]',
            append_msg:str='')->record:
        """
        make a 'record' with the matched pattern
        `append_msg` is appended to .msg
        """

        level, date, thread_id, filename, line, msg = args
        level = cls.LEVEL_MAPPING[level]
        thread_id = int(thread_id)
        line = int(line)
        if not isinstance(date, datetime):
            date = datetime.strptime(date, '%m%d %H:%M:%S.%f')
        msg += append_msg
        return record(level, date, thread_id, filename, line, msg)

    def __init__(self,
                 text_stream:'Optional[Iterable[str]]'=None,
                 hold_last:bool=True):
        self.stream = text_stream
        self.hold_last = hold_last
        self.reset()

    def __iter__(self):
        assert self.stream

        return self.process(self.stream)

    def reset(self):
        """reset state"""

        self.last_buffer = ''
        self.last_record = None

    def process(self, text_stream:'Iterable[str]')->'Iterable[record]':
        """parse glog `text_stream` into record stream"""

        self.reset()
        for buffer in text_stream:
            yield from self.parse(buffer, hold_last=self.hold_last)

    def parse(self, buffer:'Union[str, bytes]',
              hold_last=False)->'Iterable[record]':
        """
        parse block of logging lines
        if `hold_last` is True, the last record will not be emitted
        """

        if isinstance(buffer, bytes):
            buffer = buffer.decode('utf-8')

        buffer = self.last_buffer + buffer
        match_iter = GLOG_PERROR_REGEX.finditer(buffer)
        last_end = 0
        buffer_ = ''
        for match in match_iter:
            buffer_ += buffer[last_end:match.start()]
            last_end = match.end()
        buffer_ += buffer[last_end:]
        buffer = buffer_

        match_iter = GLOG_HEAD_REGEX.finditer(buffer)
        last_end = 0
        for match in match_iter:
            if self.last_record is not None:
                yield self.make_record(*self.last_record,
                                       append_msg=buffer[last_end:match.start()])
            self.last_record = self.make_record(*match.groups(), '')
            last_end = match.end()

        if hold_last:
            self.last_buffer = buffer[last_end:]
        elif self.last_record is not None:
            yield self.make_record(*self.last_record, append_msg=buffer[last_end:])
            self.reset()


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(
            description="glog filter",
            formatter_class=argparse.ArgumentDefaultsHelpFormatter,
            )
    parser.add_argument(
            'log_path', nargs=1,
            help='path to log file',
            )
    parser.add_argument(
            '--filter', '-f', action='append',
            help='filter: filter_name=args',
            )
    args = parser.parse_args()

    import filters

    from backends import tailc

    proc = tailc(args.log_path[0])
    parser = GlogParser()
    record_stream = parser.process(proc.stdout)
    if args.filter:
        for param in args.filter:
            filter_name, _, filter_param = param.partition('=')
            func = getattr(filters, filter_name + '_filter')
            filter_params = [filter_param] if filter_param else []
            record_stream = func(record_stream, *filter_params)
    msg_stream = get_msg(record_stream)
    for msg in msg_stream:
        print(msg)
