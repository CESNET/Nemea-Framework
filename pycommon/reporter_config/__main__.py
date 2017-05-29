import sys

from Config import Config
from actions.Drop import DropMsg
import logging as log

if __name__ == "__main__":
    """Run basic tests
    """
    msg = {
        "ID" : "e214d2d9-359b-443d-993d-3cc5637107a0",
        "WinEndTime" : "2016-06-21T11:25:01Z",
        "ConnCount" : 2,
        "Source" : [
            {
                "IP4" : [
                    "188.14.166.39"
                ]
            }
        ],
        "Format" : "IDEA0",
        "WinStartTime" : "2016-06-21T11:20:01Z",
        "_CESNET" : {
            "StorageTime" : 1466508305
        },
        "Target" : [
            {
                "IP4" : [
                    "195.113.165.128/25"
                ],
                "Port" : [
                    "22"
                ],
                "Proto" : [
                    "tcp",
                    "ssh"
                ],
                "Anonymised" : True
            }
        ],
        "Note" : "SSH login attempt",
        "DetectTime" : "2016-06-21T13:08:27Z",
        "Node" : [
            {
                "Name" : "cz.cesnet.mentat.warden_filer",
                "Type" : [
                    "Relay"
                ]
            },
            {
                "AggrWin" : "00:05:00",
                "Type" : [
                    "Connection",
                    "Honeypot",
                    "Recon"
                ],
                "SW" : [
                    "Kippo"
                ],
                "Name" : "cz.uhk.apate.cowrie"
            }
        ],
        "Category" : [
            "Attempt.Login"
        ]
    }

    # user did not specify config file path
    if len(sys.argv) == 1:
        conf = Config("config.yaml")
    else:
        conf = Config(sys.argv[1])

    print(conf.conf)

    log.basicConfig(level=conf.loglevel())

    try:
        conf.match(msg)
    except DropMsg as e:
        log.info("Should drop")
