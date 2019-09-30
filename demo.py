#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Sep 20 10:42:02 2019

@author: Macrobull
"""

from __future__ import division

from backends import tailc
from filters import thread_filter
from glog_parser import GlogParser, get_msg
from parsers import FrameParser, frame_parser


thread_id = None
while True:
    proc = tailc('/tmp/test.log')
    glog_parser = GlogParser()
    record_stream = glog_parser.process(proc.stdout)
    if thread_id is None:
        for record in record_stream:
            match = FrameParser.REGEX.fullmatch(record.msg)
            if match:
                frame = FrameParser.make_frame(*match.groups())
                if frame.name.startswith('Thread'):
                    thread_id = record.thread_id
                    break
    else:
        record_stream = thread_filter(record_stream, thread_id)
        break


import time
import matplotlib.pyplot as plt


plt.interactive(True)
ax = plt.subplot(title=f'Thread {thread_id}')
t1 = time.time()
series = dict()
msg_stream = get_msg(record_stream)
frame_stream = frame_parser(msg_stream)
for frame, document in frame_stream:
    print(frame.index, document)
    for key, value in document.items():
        data = series.setdefault(key, [])
        data.append(value)
        series[key] = data[-100:]
    t2 = time.time()
    if t1 + 0.05 < t2:
        ax.clear()
        cnt = 0
        for key, value in series.items():
            ax.plot(value, label=key)
            cnt += 1
            if cnt >= 2:
                break
        ax.legend(loc='best')
        t2 = time.time()
        plt.pause(max(0.01, t1 + 0.1 - t2))
        t1 = t2
