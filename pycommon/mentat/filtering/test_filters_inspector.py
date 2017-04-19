#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#-------------------------------------------------------------------------------
# This file is part of Mentat system (https://mentat.cesnet.cz/).
#
# Copyright (C) since 2011 CESNET, z.s.p.o (http://www.ces.net/)
# Use of this source is governed by the MIT license, see LICENSE file.
#-------------------------------------------------------------------------------

import os
import sys
import shutil
import unittest
from pprint import pformat, pprint

# Generate the path to custom 'lib' directory
lib = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../../lib'))
sys.path.insert(0, lib)

from idea import lite
from mentat.filtering.jpath import *
from mentat.filtering.rules import *
from mentat.filtering.gparser import MentatFilterParser
from mentat.filtering.filters import DataObjectFilter, IDEAFilterCompiler, clean_variable

#-------------------------------------------------------------------------------
# NOTE: Sorry for the long lines in this file. They are deliberate, because the
# assertion permutations are (IMHO) more readable this way.
#-------------------------------------------------------------------------------

class TestMentatDataObjectFilterInspector(unittest.TestCase):

    def test_01_current_inspector_filters(self):
        """
        Perform tests of filters currently used in mentat-inspector.py.
        """
        self.maxDiff = None

        flt = DataObjectFilter()
        cpl = IDEAFilterCompiler()
        psr = MentatFilterParser()
        psr.build()

        inspection_rules = [
            {
                "filter": "Category in ['Attempt.Login'] and Target.Port in [3389]",
                "str":    '((Category OP_IN ["Attempt.Login"]) OP_AND (Target.Port OP_IN [3389]))',
                "tests": [
                    [
                        {"Format" : "IDEA0","ID" : "e214d2d9-359b-443d-993d-3cc5637107a0","Source":[{"IP4":["188.14.166.39"]}],"Target":[{"IP4":["195.113.165.128/25"],"Port":["3389"],"Proto":["tcp","ssh"]}],"Note":"SSH login attempt","DetectTime" : "2016-06-21 13:08:27Z","Node":[{"Type":["Connection","Honeypot"],"SW":["Kippo"],"Name":"cz.uhk.apate.cowrie"}],"Category":["Attempt.Login"]},
                        True
                    ],
                    [
                        {"Format" : "IDEA0","ID" : "e214d2d9-359b-443d-993d-3cc5637107a0","Source":[{"IP4":["188.14.166.39"]}],"Target":[{"IP4":["195.113.165.128/25"],"Port":["338"],"Proto":["tcp","ssh"]}],"Note":"SSH login attempt","DetectTime" : "2016-06-21 13:08:27Z","Node":[{"Type":["Connection","Honeypot"],"SW":["Kippo"],"Name":"cz.uhk.apate.cowrie"}],"Category":["Attempt.Login"]},
                        False
                    ]
                ],
            },
            {
                "filter": "Category in ['Attempt.Login'] and (Target.Proto in ['telnet'] or Source.Proto in ['telnet'] or Target.Port in [23])",
                "str":    '((Category OP_IN ["Attempt.Login"]) OP_AND ((Target.Proto OP_IN ["telnet"]) OP_OR ((Source.Proto OP_IN ["telnet"]) OP_OR (Target.Port OP_IN [23]))))'
            },
            {
                "filter": "Category in ['Attempt.Login'] and (Target.Proto in ['ssh'] or Source.Proto in ['ssh'] or Target.Port in [22])",
                "str":    '((Category OP_IN ["Attempt.Login"]) OP_AND ((Target.Proto OP_IN ["ssh"]) OP_OR ((Source.Proto OP_IN ["ssh"]) OP_OR (Target.Port OP_IN [22]))))'
            },
            {
                "filter": "Category in ['Attempt.Login'] and (Target.Proto in ['sip', 'sip-tls'] or Source.Proto in ['sip', 'sip-tls'] or Target.Port in [5060])",
                "str":    '((Category OP_IN ["Attempt.Login"]) OP_AND ((Target.Proto OP_IN ["sip", "sip-tls"]) OP_OR ((Source.Proto OP_IN ["sip", "sip-tls"]) OP_OR (Target.Port OP_IN [5060]))))'
            },
            {
                "filter": "Category in ['Attempt.Exploit'] and (Target.Proto in ['sip', 'sip-tls'] or Source.Proto in ['sip', 'sip-tls'] or Target.Port in [5060])",
                "str":    '((Category OP_IN ["Attempt.Exploit"]) OP_AND ((Target.Proto OP_IN ["sip", "sip-tls"]) OP_OR ((Source.Proto OP_IN ["sip", "sip-tls"]) OP_OR (Target.Port OP_IN [5060]))))'
            },
            {
                "filter": "Category in ['Attempt.Exploit'] and Target.Port in [23]",
                "str":    '((Category OP_IN ["Attempt.Exploit"]) OP_AND (Target.Port OP_IN [23]))'
            },
            {
                "filter": "Category in ['Attempt.Exploit'] and (Target.Port in [80, 443] or Source.Proto in ['http', 'https', 'http-alt'] or Target.Proto in ['http', 'https', 'http-alt'])",
                "str":    '((Category OP_IN ["Attempt.Exploit"]) OP_AND ((Target.Port OP_IN [80, 443]) OP_OR ((Source.Proto OP_IN ["http", "https", "http-alt"]) OP_OR (Target.Proto OP_IN ["http", "https", "http-alt"]))))'
            },
            {
                "filter": "Category in ['Attempt.Exploit'] and (Target.Port in [3306] or Source.Proto in ['mysql'] or Target.Proto in ['mysql'])",
                "str":    '((Category OP_IN ["Attempt.Exploit"]) OP_AND ((Target.Port OP_IN [3306]) OP_OR ((Source.Proto OP_IN ["mysql"]) OP_OR (Target.Proto OP_IN ["mysql"]))))'
            },
            {
                "filter": "Category in ['Attempt.Exploit'] and (Target.Port in [445] or Source.Proto in ['microsoft-ds', 'smb'] or Target.Proto in ['microsoft-ds', 'smb'])",
                "str":    '((Category OP_IN ["Attempt.Exploit"]) OP_AND ((Target.Port OP_IN [445]) OP_OR ((Source.Proto OP_IN ["microsoft-ds", "smb"]) OP_OR (Target.Proto OP_IN ["microsoft-ds", "smb"]))))'
            },
            {
                "filter": "Category in ['Attempt.Exploit'] and (Target.Port in [135] or Source.Proto in ['loc-srv', 'epmap'] or Target.Proto in ['loc-srv', 'epmap'])",
                "str":    '((Category OP_IN ["Attempt.Exploit"]) OP_AND ((Target.Port OP_IN [135]) OP_OR ((Source.Proto OP_IN ["loc-srv", "epmap"]) OP_OR (Target.Proto OP_IN ["loc-srv", "epmap"]))))'
            },
            {
                "filter": "Category in ['Attempt.Exploit'] and (Target.Port in [1900] or Source.Proto in ['upnp', 'ssdp'] or Target.Proto in ['upnp', 'ssdp'])",
                "str":    '((Category OP_IN ["Attempt.Exploit"]) OP_AND ((Target.Port OP_IN [1900]) OP_OR ((Source.Proto OP_IN ["upnp", "ssdp"]) OP_OR (Target.Proto OP_IN ["upnp", "ssdp"]))))'
            },
            {
                "filter": "Category in ['Attempt.Exploit'] and (Target.Port in [20, 21, 989, 990] or Source.Proto in ['ftp', 'ftp-data', 'ftps', 'ftps-data'] or Target.Proto in ['ftp', 'ftp-data', 'ftps', 'ftps-data'])",
                "str":    '((Category OP_IN ["Attempt.Exploit"]) OP_AND ((Target.Port OP_IN [20, 21, 989, 990]) OP_OR ((Source.Proto OP_IN ["ftp", "ftp-data", "ftps", "ftps-data"]) OP_OR (Target.Proto OP_IN ["ftp", "ftp-data", "ftps", "ftps-data"]))))'
            },
            {
                "filter": "Category in ['Attempt.Exploit'] and (Target.Port in [1433, 1434] or Source.Proto in ['ms-sql-s', 'ms-sql-m'] or Target.Proto in ['ms-sql-s', 'ms-sql-m'])",
                "str":    '((Category OP_IN ["Attempt.Exploit"]) OP_AND ((Target.Port OP_IN [1433, 1434]) OP_OR ((Source.Proto OP_IN ["ms-sql-s", "ms-sql-m"]) OP_OR (Target.Proto OP_IN ["ms-sql-s", "ms-sql-m"]))))'
            },
            {
                "filter": "Category in ['Attempt.Exploit'] and (Target.Port in [42] or Source.Proto in ['nameserver'] or Target.Proto in ['nameserver'])",
                "str":    '((Category OP_IN ["Attempt.Exploit"]) OP_AND ((Target.Port OP_IN [42]) OP_OR ((Source.Proto OP_IN ["nameserver"]) OP_OR (Target.Proto OP_IN ["nameserver"]))))'
            },
            {
                "filter": "Category in ['Attempt.Exploit'] and Node.SW in ['Dionaea']",
                "str":    '((Category OP_IN ["Attempt.Exploit"]) OP_AND (Node.SW OP_IN ["Dionaea"]))'
            },
            {
                "filter": "Category in ['Availability.DoS', 'Availability.DDoS'] and (Target.Proto in ['dns', 'domain'] or Source.Proto in ['dns', 'domain'] or Target.Port in [53] or Source.Port in [53])",
                "str":    '((Category OP_IN ["Availability.DoS", "Availability.DDoS"]) OP_AND ((Target.Proto OP_IN ["dns", "domain"]) OP_OR ((Source.Proto OP_IN ["dns", "domain"]) OP_OR ((Target.Port OP_IN [53]) OP_OR (Source.Port OP_IN [53])))))'
            },
            {
                "filter": "Category in ['Availability.DDoS'] and Node.Type in ['Flow'] and Node.Type in ['Statistical']",
                "str":    '((Category OP_IN ["Availability.DDoS"]) OP_AND ((Node.Type OP_IN ["Flow"]) OP_AND (Node.Type OP_IN ["Statistical"])))'
            },
            {
                "filter": "Category in ['Abusive.Spam'] and Node.SW in ['UCEPROT']",
                "str":    '((Category OP_IN ["Abusive.Spam"]) OP_AND (Node.SW OP_IN ["UCEPROT"]))'
            },
            {
                "filter": "Category in ['Abusive.Spam'] and Node.SW in ['Fail2Ban', 'IntelMQ']",
                "str":    '((Category OP_IN ["Abusive.Spam"]) OP_AND (Node.SW OP_IN ["Fail2Ban", "IntelMQ"]))'
            },
            {
                "filter": "Category in ['Vulnerable.Config'] and (Source.Proto in ['qotd'] or Source.Port in [17])",
                "str":    '((Category OP_IN ["Vulnerable.Config"]) OP_AND ((Source.Proto OP_IN ["qotd"]) OP_OR (Source.Port OP_IN [17])))'
            },
            {
                "filter": "Category in ['Vulnerable.Config'] and Source.Proto in ['ssdp']",
                "str":    '((Category OP_IN ["Vulnerable.Config"]) OP_AND (Source.Proto OP_IN ["ssdp"]))'
            },
            {
                "filter": "Category in ['Vulnerable.Config'] and (Source.Proto in ['ntp'] or Source.Port in [123])",
                "str":    '((Category OP_IN ["Vulnerable.Config"]) OP_AND ((Source.Proto OP_IN ["ntp"]) OP_OR (Source.Port OP_IN [123])))'
            },
            {
                "filter": "Category in ['Vulnerable.Config'] and (Source.Proto in ['domain'] or Source.Port in [53])",
                "str":    '((Category OP_IN ["Vulnerable.Config"]) OP_AND ((Source.Proto OP_IN ["domain"]) OP_OR (Source.Port OP_IN [53])))'
            },
            {
                "filter": "Category in ['Vulnerable.Config'] and (Source.Proto in ['netbios-ns'] or Source.Port in [137])",
                "str":    '((Category OP_IN ["Vulnerable.Config"]) OP_AND ((Source.Proto OP_IN ["netbios-ns"]) OP_OR (Source.Port OP_IN [137])))'
            },
            {
                "filter": "Category in ['Vulnerable.Config'] and (Source.Proto in ['ipmi'] or Source.Port in [623])",
                "str":    '((Category OP_IN ["Vulnerable.Config"]) OP_AND ((Source.Proto OP_IN ["ipmi"]) OP_OR (Source.Port OP_IN [623])))'
            },
            {
                "filter": "Category in ['Vulnerable.Config'] and (Source.Proto in ['chargen'] or Source.Port in [19])",
                "str":    '((Category OP_IN ["Vulnerable.Config"]) OP_AND ((Source.Proto OP_IN ["chargen"]) OP_OR (Source.Port OP_IN [19])))'
            },
            {
                "filter": "Category in ['Anomaly.Traffic']",
                "str":    '(Category OP_IN ["Anomaly.Traffic"])'
            },
            {
                "filter": "Category in ['Anomaly.Connection'] and Source.Type in ['Booter']",
                "str":    '((Category OP_IN ["Anomaly.Connection"]) OP_AND (Source.Type OP_IN ["Booter"]))'
            },
            {
                "filter": "Category in ['Intrusion.Botnet'] and Source.Type in ['Botnet']",
                "str":    '((Category OP_IN ["Intrusion.Botnet"]) OP_AND (Source.Type OP_IN ["Botnet"]))'
            },
            {
                "filter": "Category in ['Recon.Scanning']",
                "str":    '(Category OP_IN ["Recon.Scanning"])'
            }
        ]

        for ir in inspection_rules:
            rule = psr.parse(ir['filter'])
            rule = cpl.compile(rule)
            self.assertEqual(str(rule), ir['str'])
            if 'tests' in ir:
                for t in ir['tests']:
                    msg_idea = lite.Idea(t[0])
                    self.assertEqual([ir['filter'], flt.filter(rule, msg_idea)], [ir['filter'], t[1]])


if __name__ == '__main__':
    unittest.main()
