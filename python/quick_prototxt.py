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


def parse_prototxt(s:str)->'Mapping[str, Any]':
    """direct parse ProtoBuffer text format"""

    unames = OrderedDict()

    def replace_key(s:str):
        t = ''
        start = 0
        for m in re.finditer(r'(\w+)\s*:', s):
            okey = m.groups()[0]
            nkey = okey + '@' + uuid4().hex
            unames[nkey] = okey
            t += s[start:m.start()]
            t += nkey + ':'
            start = m.end()
        return t + s[start:]

    def restore_key(nobj:dict):
        oobj = type(nobj)()
        for nkey, nsub in nobj.items():
            okey = unames[nkey]
            nsub = restore_key(nsub) if isinstance(nsub, dict) else nsub
            osub = oobj.get(okey, None)
            if osub is None:
                oobj[okey] = nsub
            else:
                if not isinstance(osub, list):
                    oobj[okey] = [osub]
                oobj[okey].append(nsub)
        return oobj

    s = re.sub(r'\s+\{', ': {', s) # add : for field
    s = replace_key(s)
    s = re.sub(r'(?<=[^\{])\n', ',\n', s) # add, for flow mapping
    s = '{\n  ' + s.strip().replace('\n', '\n  ') + '\n}' # add root flow mapping brace
    return restore_key(yaml.safe_load(s))
