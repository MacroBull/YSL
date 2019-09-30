#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Sep 20 10:42:02 2019

@author: Macrobull
"""

from __future__ import division

import io, re
import yaml

from collections import namedtuple


class TextStreamIO(io.TextIOBase):
    """
    wrap text line iterator for IoBase
    if `force_line_size` is True, the last complete line is ensured to be returned in 'read()'
    """

    name :str = '(TextStream)'

    def __init__(self, text_stream:'Iterable[str]',
                 force_line_size:bool=False):
        self.stream = text_stream
        self.force_line_size = force_line_size
        self.buffer = io.StringIO()

    def read(self, size:int=-1)->str:
        buffer = self.buffer.read(size)
        if size < 0:
            for line in self.stream:
                buffer += line
            return buffer

        if len(buffer) < size:
            while len(buffer) < size:
                try:
                    buffer += next(self.stream)
                except StopIteration:
                    # fewer than size bytes may be returned
                    break
            self.buffer.seek(0)
            self.buffer.truncate()
            if not self.force_line_size:
                self.buffer.write(buffer[size:])
                self.buffer.seek(0)
                buffer = buffer[:size]
        return buffer

    def readline(self)->str:
        buffer = self.buffer.readline()
        pos = self.buffer.tell()
        self.buffer.seek(0, 2)
        if pos == self.buffer.tell():
            try:
                buffer += next(self.stream)
            except StopIteration:
                # fewer than size bytes may be returned
                pass
            self.buffer.seek(0)
            self.buffer.truncate()
        else:
            self.buffer.seek(pos)
        return buffer


class StreamParser(object):
    """
    Abstract stream parser
        if `stream` given, parser is iterable within this stream
    """

    def __init__(self,
                 stream:'Optional[Iterable[Any]]'=None):
        self.stream = stream
        self.reset()

    def __iter__(self)->'Iterable[Any]':
        assert self.stream

        return self.process(self.stream)

    def reset(self):
        """reset parser state for a new stream"""

        pass

    def process(self, stream:'Iterable[Any]')->'Iterable[Any]':
        """process on `stream`"""

        self.reset()
        for buffer in stream:
            yield self.parse(buffer)

    def parse(self, buffer:'Any')->'Any':
        """parse on `buffer`"""

        return buffer


FRAME_FIELDS :tuple = ('name', 'index')

frame :type = namedtuple('frame', FRAME_FIELDS)


class FrameParser(StreamParser):
    """
    FrameParser parses YSL::*Frame from the text stream and bypass the stream
    (name, index) is stored and can be fetched with pop_frame
    """

    PATTERN :str = r'--- #\s+-*\s+(.+)\s+-*\s+# ---\n'
    REGEX = re.compile(PATTERN)

    @staticmethod
    def make_frame(*args:'Tuple[Any, ...]')->frame:
        """make frame"""

        text, = args
        name, _, index = text.partition(': ')
        index = int(index) if index else None
        return frame(name, index)

    def reset(self):
        self.frame_queue = []

    def parse(self, line:str)->str:
        match = self.REGEX.fullmatch(line)
        if match is not None:
            self.frame_queue.append(self.make_frame(*match.groups()))
        elif not self.frame_queue:
            self.frame_queue.append(frame('', -1))
        return line

    def pop_frame(self):
        """pop first frame in the queue"""

        assert self.frame_queue, "pop frame before any frame is resolved"

        return self.frame_queue.pop(0)


def frame_parser(
        text_stream:'Iterable[str]',
        persistent:bool=False) -> 'Iterable[Tuple[str, Any]]':
    """
    YSL Yaml frame parser, yield each (frame, document)
    raise 'yaml.YAMLError' if any yaml parser error encountered and persistent is False
    """

    frame_parser = FrameParser()
    text_stream = frame_parser.process(text_stream)
    io_stream = TextStreamIO(text_stream, force_line_size=True)
    while True:
        try:
            frame_parser.reset()
            for document in yaml.safe_load_all(io_stream):
                yield frame_parser.pop_frame(), document
        except yaml.YAMLError as e:
            if persistent:
                continue
            else:
                raise e


if __name__ == '__main__':
    from backends import tailc
    from glog_parser import GlogParser, get_msg

    proc = tailc('/tmp/test.log')
    glog_parser = GlogParser()
    record_stream = glog_parser.process(proc.stdout)
    msg_stream = get_msg(record_stream)
    frame_stream = frame_parser(msg_stream)
    for f, d in frame_stream:
        print(f)
        print(d)
