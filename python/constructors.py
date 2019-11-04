#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Sep 20 10:10:45 2019

@author: Macrobull
"""

from __future__ import absolute_import, division, unicode_literals

import yaml

from yaml import Node
from yaml.constructor import BaseConstructor, FullConstructor, SafeConstructor


class LogConstructor(SafeConstructor):
    """YSL overload"""


class LogLoader(
        yaml.reader.Reader, yaml.scanner.Scanner, yaml.parser.Parser, yaml.composer.Composer,
        LogConstructor,
        yaml.resolver.Resolver):
    """yaml loader for YSL"""

    def __init__(self, stream):
        yaml.reader.Reader.__init__(self, stream)
        yaml.scanner.Scanner.__init__(self)
        yaml.parser.Parser.__init__(self)
        yaml.composer.Composer.__init__(self)
        LogConstructor.__init__(self)
        yaml.resolver.Resolver.__init__(self)


def construct_path(
        constructor:BaseConstructor, node:Node,
        expanduser:bool=False)->'Path':
    """construct path scalar"""

    from pathlib import Path

    ret = Path(constructor.construct_scalar(node))
    if expanduser:
        ret = ret.expanduser()
    return ret.resolve()


def construct_tensor(
        constructor:BaseConstructor, node:Node,
        tensor_cls:type=list)->'Any':
    """construct tensor scalar"""

    return tensor_cls(yaml.load(constructor.construct_scalar(node), Loader=LogLoader))


def construct_pb_message(
        constructor:BaseConstructor, node:Node,
        message_cls:'Optional[type]'=None)->'Union[google.protobuf.Message, Mapping[str, Any]]':
    """construct protobuf message scalar"""

    ret = constructor.construct_scalar(node)
    if message_cls is None:
        from quick_prototxt import parse_prototxt

        return parse_prototxt(ret)
    else:
        from google.protobuf import text_format

        return text_format.Parse(ret, message_cls())


LogConstructor.add_constructor('!complex', FullConstructor.construct_python_complex)
LogConstructor.add_constructor('!path', construct_path)
LogConstructor.add_constructor('!tensor', construct_tensor)
LogConstructor.add_constructor('!pb2_message', construct_pb_message)
LogConstructor.add_constructor('!pb3_message', construct_pb_message)

if __name__ == '__main__':
    s = 'Complex: !complex 1+2j'
    print(yaml.load(s, Loader=LogLoader))

    s = ('MatExpr: !tensor |\n'
         '  [[[0, 0, 0], [0, 0, 0], [0, 0, 0]],\n'
         '   [[0, 0, 0], [0, 0, 0], [0, 0, 0]],\n'
         '   [[0, 0, 0], [0, 0, 0], [0, 0, 0]]]\n'
         )
    print(yaml.load(s, Loader=LogLoader))

    s = ('matrix:\n'
         '  - [1, 0, 0, 0, 0, 0, 0, 0]\n'
         '  - [0, 1, 0, 0, 0, 0, 0, 0]\n'
         '  - [0, 0, 1, 0, 0, 0, 0, 0]\n'
         '  - [0, 0, 0, 1, 0, 0, 0, 0]\n'
         '  - [0, 0, 0, 0, 1, 0, 0, 0]\n'
         '  - [0, 0, 0, 0, 0, 1, 0, 0]\n'
         '  - [0, 0, 0, 0, 0, 0, 1, 0]\n'
         '  - [0, 0, 0, 0, 0, 0, 0, 1]\n'
         )
    print(yaml.load(s, Loader=LogLoader))
