#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Sep 20 10:10:45 2019

@author: Macrobull
"""

from __future__ import division

import ast
import yaml

DEFAULT_LOADER :type = yaml.SafeLoader


def tensor_constructor(
	constructor:yaml.BaseLoader, node:yaml.Node,
	tensor_cls:type=list)->'Any':
	"""construct tensor scalar"""

	return tensor_cls(ast.literal_eval(constructor.construct_scalar(node)))


def pb2_constructor(
	constructor:yaml.BaseLoader, node:yaml.Node,
	message_cls:'Optional[type]'=None)->'Any':
	"""construct protobuf 2 scalar"""

	return constructor.construct_scalar(node) # TODO: implement this


if DEFAULT_LOADER != yaml.FullLoader:
	yaml.add_constructor('!complex', yaml.constructor.FullConstructor.construct_python_complex, Loader=DEFAULT_LOADER)

yaml.add_constructor('!tensor', tensor_constructor, Loader=DEFAULT_LOADER)

if __name__ == '__main__':
	from io import StringIO

	stream = StringIO()

	stream.seek(0)
	stream.write('Complex: !complex 1+2j')
	stream.seek(0)
	print(yaml.load(stream, Loader=DEFAULT_LOADER))

	stream.seek(0)
	stream.write('MatExpr: !tensor |\n'
		'  [[[0, 0, 0], [0, 0, 0], [0, 0, 0]],\n'
		'   [[0, 0, 0], [0, 0, 0], [0, 0, 0]],\n'
		'   [[0, 0, 0], [0, 0, 0], [0, 0, 0]]]\n')
	stream.seek(0)
	print(yaml.load(stream, Loader=DEFAULT_LOADER))

	stream.seek(0)
	stream.write('matrix:\n'
		'  - [1, 0, 0, 0, 0, 0, 0, 0]\n'
		'  - [0, 1, 0, 0, 0, 0, 0, 0]\n'
		'  - [0, 0, 1, 0, 0, 0, 0, 0]\n'
		'  - [0, 0, 0, 1, 0, 0, 0, 0]\n'
		'  - [0, 0, 0, 0, 1, 0, 0, 0]\n'
		'  - [0, 0, 0, 0, 0, 1, 0, 0]\n'
		'  - [0, 0, 0, 0, 0, 0, 1, 0]\n'
		'  - [0, 0, 0, 0, 0, 0, 0, 1]\n')
	stream.seek(0)
	print(yaml.load(stream, Loader=DEFAULT_LOADER))