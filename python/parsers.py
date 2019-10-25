#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Sep 20 10:42:02 2019

@author: Macrobull
"""

from __future__ import division

import io, logging, re
import yaml

from collections import namedtuple


logger = logging.getLogger(__name__)


class TextStreamIO(io.TextIOBase):
    """
    wrap text line iterator for IoBase
    if `force_readline` is True, any read(size > 0) will equally be readline()
    """

    name :str = '(TextStream)'

    def __init__(self, text_stream:'Iterable[str]',
                 force_readline:bool=False):
        self.stream = text_stream
        self.force_readline = force_readline
        self.buffer = io.StringIO()

    def readable(self):
        return True

    def read(self, size:int=-1)->str:
        if size == 0:
            return ''

        if size < 0:
            buffer = self.buffer.read(size)
            self.buffer.seek(0)
            self.buffer.truncate()
            for line in self.stream:
                buffer += line
            return buffer

        if self.force_readline:
            return self.readline()

        buffer = self.buffer.read(size)
        if len(buffer) < size:
            self.buffer.seek(0)
            self.buffer.truncate()
            while len(buffer) < size:
                try:
                    buffer += next(self.stream)
                except StopIteration:
                    # fewer than size bytes may be returned
                    break
            self.buffer.write(buffer[size:])
            self.buffer.seek(0)
            buffer = buffer[:size]
        return buffer

    def readline(self)->str:
        buffer = self.buffer.readline() # buffer always endswith '\n' or is empty
        if not buffer:
            self.buffer.seek(0)
            self.buffer.truncate()
            try:
                buffer = next(self.stream)
            except StopIteration:
                pass
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

#    def fast_forward(self, document_stream):
#        """fast forward on 'document_stream'"""
#
#        frame = None
#        document = None
#        while len(self.frame_queue) != 1:
#            document = next(document_stream)
#            frame = self.pop_frame()
#            print(self.frame_queue)
#        return frame, document


def frame_parser(
        text_stream:'Iterable[str]',
        yaml_loader_cls:type=yaml.SafeLoader,
        persistent:bool=False) -> 'Iterable[Tuple[str, Any]]':
    """
    YSL Yaml frame parser, yield each (frame, document)
    raise 'yaml.YAMLError' if any yaml parser error encountered and persistent is False
    """

    frame_parser_ = FrameParser()
    text_stream = frame_parser_.process(text_stream)
    io_stream = TextStreamIO(text_stream, force_readline=True)
    while True:
        try:
            frame_parser_.reset()
            for document in yaml.load_all(io_stream, Loader=yaml_loader_cls):
                yield frame_parser_.pop_frame(), document
        except yaml.YAMLError as e:
            if persistent:
                logger.warn('got exception:\n%s\nparser will be reseted', e)
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
#    frame_parser_ = FrameParser()
#    text_stream = frame_parser_.process(msg_stream)
#    io_stream = TextStreamIO(text_stream, force_readline=True)
#    doc_stream = yaml.safe_load_all(io_stream)
#    for d in doc_stream:
#        f = frame_parser_.pop_frame()
    for f, d in frame_stream:
        print(f)
        print(d)
