#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun Sep 15 22:09:46 2019

@author: Macrobull
"""

from __future__ import absolute_import, division, unicode_literals

import re
import yaml

from collections import OrderedDict
from uuid import uuid4


DELIMITER :str = '@'


def load_prototxt(s:str)->'Dict[str, Any]':
    """direct deserialize from ProtoBuffer text format"""

    unames = OrderedDict()

    def replace_key(s):
        t = ''
        start = 0
        for m in re.finditer(r'(\w+)\s*:', s): # HINT: ^\s*\w+\s*:
            ok, = m.groups()
            nk = ok + DELIMITER + uuid4().hex
            unames[nk] = ok
            t += s[start:m.start()]
            t += nk + ':'
            start = m.end()
        return t + s[start:]

    def restore_key(no):
        oo = type(no)()
        for nk, nv in no.items():
            ok = unames[nk]
            nv = restore_key(nv) if isinstance(nv, dict) else nv
            ov = oo.get(ok, None)
            if ov is None:
                oo[ok] = nv
            else:
                if not isinstance(ov, list):
                    oo[ok] = [ov]
                oo[ok].append(nv)
        return oo

    s = re.sub(r'\{\s+\n', '{\n', s) # remove space between { and \n
    s = re.sub(r'\s*\{', ': {', s) # add : for field, HINT: \w+\s*{
    s = replace_key(s)
    s = re.sub(r'(?<=[^\{])\n', ',\n', s) # add, for flow mapping
    # s = '{\n  ' + s.strip().replace('\n', '\n  ') + '\n}' # add root flow mapping brace
    s = '{' + s + '}' # simply
    return restore_key(yaml.safe_load(s))


def dump_prototxt(o:'Mapping[str, Any]')->str:
    """direct serialize to ProtoBuffer text format"""

    list_clss = (list, tuple, set)
    indent = 2

    def replace_key(oo):
        no = type(oo)()
        for ok, ov in oo.items():
            if isinstance(ov, dict):
                no[ok] = replace_key(ov)
            elif isinstance(ov, list_clss):
                for idx, oi in enumerate(ov):
                    assert not isinstance(oi, list_clss), 'value cannot be unnamed list'

                    nk = ok + DELIMITER + str(idx) # make key ordered
                    ni = replace_key(oi) if isinstance(oi, dict) else oi
                    no[nk] = ni
            else:
                no[ok] = ov
        return no

    def restore_key(s):
        t = ''
        start = 0
        for m in re.finditer(r'(\w+)' + DELIMITER + r'\d+\s*:', s): # HINT: ^\s*\w+@\d+\s*:
            ok, = m.groups()
            t += s[start:m.start()]
            # t += ok + ':'
            t += ok # simply
            start = m.end()
        return t + s[start:]

    def add_mapping_end_break(s):
        t = ''
        start = 0
        for m in re.finditer(r'(?<=\n)(\s+)(.+?)(}+)(?=\n)', s):
            spaces, content, braces = m.groups()
            assert len(spaces) >= len(braces) * indent

            t += s[start:m.start()]
            t += spaces + content
            for brace in braces:
                spaces = spaces[:-indent]
                t += '\n' + spaces + brace
            start = m.end()
        return t + s[start:]

    o = replace_key(o)
    # HINT: ~ canonical=True
    s = yaml.safe_dump(o, default_flow_style=True, indent=indent, width=(indent * 2 + 1))
    s = s.strip()[1: -1].replace('\n  ', '\n') # remove root flow mapping brace
    s = restore_key(s)
    s = s.replace(',\n', '\n').replace(': {', ' {') # remove , and :
    s = add_mapping_end_break(s + '\n')
    return s
