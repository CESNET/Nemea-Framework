import sys, os.path
import argparse
import json
import pytrap
from time import time, gmtime
from uuid import uuid4
from datetime import datetime, timedelta
import re
import logging
import signal

from reporter_config import Config
from reporter_config import Parser
from reporter_config.actions.Drop import DropMsg
FORMAT="%(asctime)s %(module)s:%(filename)s:%(lineno)d:%(message)s"

logger = logging.getLogger(__name__)

def getRandomId():
    """Return unique ID of IDEA message. It is done by UUID in this implementation."""
    return str(uuid4())

def setAddr(idea_field, addr):
    """Set IP address into 'idea_field'.
This method automatically recognize IPv4 vs IPv6 and sets the correct information into the IDEA message.
If there is already a list of addresses, new `addr` is appended.
Usage: setAddr(idea['Source'][0], rec.SRC_IP)"""

    if addr.isIPv4():
        if 'IP4' in idea_field:
            idea_field['IP4'].append(str(addr))
        else:
            idea_field['IP4'] = [str(addr)]
    else:
        if 'IP6' in idea_field:
            idea_field['IP6'].append(str(addr))
        else:
            idea_field['IP6'] = [str(addr)]

def getIDEAtime(unirecField = None):
    """Return timestamp in IDEA format (string).
    If unirecField is provided, it will convert it into correct format.
    Otherwise, current time is returned.

    Example:
    >>> getIDEAtime(pytrap.UnirecTime(1234567890))
    '2009-02-13T23:31:30Z'
    """

    if unirecField:
        # Convert UnirecTime
        ts = unirecField.toDatetime()
        return ts.strftime('%Y-%m-%dT%H:%M:%SZ')
    else:
        g = gmtime()
        iso = '%04d-%02d-%02dT%02d:%02d:%02dZ' % g[0:6]
    return iso


def parseRFCtime(time_str):
    """Parse time_str in RFC 3339 format and return it as native datetime in UTC.

    Example:

    >>> parseRFCtime('2019-03-11T14:59:54Z')
    datetime.datetime(2019, 3, 11, 14, 59, 54)
    """
    # Regex for RFC 3339 time format
    timestamp_re = re.compile(r"^([0-9]{4})-([0-9]{2})-([0-9]{2})[Tt ]([0-9]{2}):([0-9]{2}):([0-9]{2})(?:\.([0-9]+))?([Zz]|(?:[+-][0-9]{2}:[0-9]{2}))$")
    res = timestamp_re.match(time_str)
    if res is not None:
        year, month, day, hour, minute, second = (int(n or 0) for n in res.group(*range(1, 7)))
        us_str = (res.group(7) or "0")[:6].ljust(6, "0")
        us = int(us_str)
        zonestr = res.group(8)
        zoneoffset = 0 if zonestr in ('z', 'Z') else int(zonestr[:3])*60 + int(zonestr[4:6])
        zonediff = timedelta(minutes=zoneoffset)
        return datetime(year, month, day, hour, minute, second, us) - zonediff
    else:
        raise ValueError("Wrong timestamp format")


# TODO: resolve argument parsing and help in Python modules
# Ideally it should all be done in Python using overloaded ArgParse

# TODO: allow setting library verbose mode

# Template of module description
desc_template = """
TRAP module - Reporter
===========================================
Name: {name}
Inputs: 1
Outputs: 0 or 1 (depending on --trap parameter)
Description:
  {original_desc}Required format of input:
    {type}: "{fmt}"

  All '<something>2idea' modules convert reports from various detectors to Intrusion Detection Extensible Alert (IDEA) format.
  The IDEA messages may be send or stored using various actions, see http://nemea.liberouter.org/reporting/ for more information.
"""

DEFAULT_NODE_NAME = "undefined"

trap = pytrap.TrapCtx()

def signal_h(signal, f):
    global trap
    trap.terminate()

def check_valid_timestamps(idea, dpast=1, dfuture=0):
    """
    Return True if EventTime, CeaseTime, and DetectTime are in the interval (CreateTime - dpast, CreateTime + dfuture).

    :param idea: dict with filled IDEA message
    :param dpast: int maximal number of days into past
    :param dfuture: int maximal number of days into future

    >>> idea = {"CreateTime": "2019-03-11T15:00:00Z", "DetectTime": "2019-03-11T14:59:00Z",
    ... "EventTime": "2019-03-11T14:57:00Z", "CeaseTime": "2019-03-11T14:58:50Z"}
    >>> check_valid_timestamps(idea, 1, 0)
    True
    >>> idea = {"CreateTime": "2019-03-11T15:00:00Z", "DetectTime": "2019-03-11T15:00:00Z",
    ... "EventTime": "2019-03-11T14:57:00Z", "CeaseTime": "2019-03-11T14:58:50Z"}
    >>> check_valid_timestamps(idea, 1, 0)
    True
    >>> # One day tolerance for the future
    >>> idea = {"CreateTime": "2019-03-11T15:00:00Z", "DetectTime": "2019-03-11T15:10:00Z",
    ... "EventTime": "2019-03-11T14:57:00Z", "CeaseTime": "2019-03-11T14:58:50Z"}
    >>> check_valid_timestamps(idea, 1, 1)
    True
    >>> # DetecTime from future
    >>> idea = {"CreateTime": "2019-03-11T15:00:00Z", "DetectTime": "2019-03-11T15:10:00Z",
    ... "EventTime": "2019-03-11T14:57:00Z", "CeaseTime": "2019-03-11T14:58:50Z"}
    >>> check_valid_timestamps(idea, 1, 0)
    False
    >>> # CreateTime from past
    >>> idea = {"CreateTime": "2019-03-10T15:00:00Z", "DetectTime": "2019-03-11T14:59:00Z",
    ... "EventTime": "2019-03-11T14:57:00Z", "CeaseTime": "2019-03-11T14:58:50Z"}
    >>> check_valid_timestamps(idea, 1, 0)
    False

    """
    et = idea.get("EventTime", None)
    ct = idea.get("CeaseTime", None)
    dt = idea.get("DetectTime", None)
    create = idea.get("CreateTime", None)
    if not create:
        return True
    else:
        create = parseRFCtime(create)
    deltapast = create - timedelta(dpast)
    deltafuture = create + timedelta(dfuture)
    for t in [et, ct, dt]:
        if t:
            # convert timestamp to datetime
            t = parseRFCtime(t)
            # check allowed interval
            if t < deltapast or t > deltafuture:
                return False
    return True


def Run(module_name, module_desc, req_type, req_format, conv_func, arg_parser = None):
    """Run the main loop of the reporter module called `module_name` with `module_desc` (used in help).

    The module requires data format of `req_type` type and `req_format` specifier - these must be given by author of the module.

    `conv_func(rec, args)` is a callback function that must translate given incoming alert `rec` (typically in UniRec according to `req_type`) into IDEA message. `args` contains CLI arguments parsed by ArgumentParser. `conv_func` must return dict().
    """
    global trap

    # *** Parse command-line arguments ***
    if arg_parser is None:
        arg_parser = argparse.ArgumentParser()
    arg_parser.formatter_class = argparse.RawDescriptionHelpFormatter

    # Set description
    arg_parser.description = str.format(desc_template,
            name=module_name + "2idea",
            type={pytrap.FMT_RAW:'raw', pytrap.FMT_UNIREC:'UniRec', pytrap.FMT_JSON:'JSON'}.get(req_type,'???'),
            fmt=req_format,
            original_desc = module_desc+"\n\n  " if module_desc else "",
            )

    # Add arguments defining outputs
    # TRAP output
    arg_parser.add_argument('-T', '--trap', action='store_true',
            help='Enable output via TRAP interface (JSON type with format id "IDEA"). Parameters are set using "-i" option as usual.')
    # Config file
    arg_parser.add_argument('-c', '--config', metavar="FILE", default="./config.yaml", type=str,
            help='Specify YAML config file path which to load.')
    arg_parser.add_argument('-d', '--dry',  action='store_true',
            help="""Do not run, just print loaded config.""")
    # Warden3 output
    arg_parser.add_argument('-W', '--warden', metavar="CONFIG_FILE",
            help='Send IDEA messages to Warden server. Load configuration of Warden client from CONFIG_FILE.')

    # Other options
    arg_parser.add_argument('-n', '--name', metavar='NODE_NAME',
            help='Name of the node, filled into "Node.Name" element of the IDEA message.')

    arg_parser.add_argument('-v', '--verbose', metavar='VERBOSE_LEVEL', default=3, type=int,
            help="""Enable verbose mode (may be used by some modules, common part doesn't print anything).\nLevel 1 logs everything, level 5 only critical errors. Level 0 doesn't log.""")

    arg_parser.add_argument('-D', '--dontvalidate', action='store_true', default=False,
            help="""Disable timestamp validation, i.e. allow timestamps to be far in the past or future.""")

    # TRAP parameters
    trap_args = arg_parser.add_argument_group('Common TRAP parameters')
    trap_args.add_argument('-i', metavar="IFC_SPEC", help='See http://nemea.liberouter.org/trap-ifcspec/ for more information.')
    # Parse arguments
    args = arg_parser.parse_args()

    # Set log level
    logging.basicConfig(level=(args.verbose*10), format=FORMAT)

    parsed_config = Config.Parser(args.config)
    if not parsed_config or not parsed_config.config:
        print("error: Parsing configuration file failed.")
        sys.exit(1)

    # Check if node name is set if Warden output is enabled
    if not args.name:
        args.name = ".".join([parsed_config.get("namespace", "com.example"), module_name])
    else:
        logger.warning("Node name is specified as '-n' argument.")
    logger.info("Node name: %s", args.name)

    if not args.dry:
        if not args.i:
            arg_parser.print_usage()
            print("error: the following arguments are required: -i")
            sys.exit(1)

        # *** Initialize TRAP ***
        logger.info("Trap arguments: %s", args.i)
        trap.init(["-i", args.i], 1, 1 if args.trap else 0)
        #trap.setVerboseLevel(3)
        signal.signal(signal.SIGINT, signal_h)

        # Set required input format
        trap.setRequiredFmt(0, req_type, req_format)
        if args.trap:
           trap.setDataFmt(0, pytrap.FMT_JSON, "IDEA")

    # *** Create output handles/clients/etc ***
    wardenclient = None

    if args.warden:
        try:
            import warden_client
        except:
            logger.error("There is no available warden_client python module.  Install it or remove '--warden' from the module's arguments.")
            sys.exit(1)
        config = warden_client.read_cfg(args.warden)
        config['name'] = args.name
        wardenclient = warden_client.Client(**config)

    # Initialize configuration
    config = Config.Config(parsed_config, trap = trap, warden = wardenclient)


    if not args.dry:
        # *** Main loop ***
        URInputTmplt = None
        if req_type == pytrap.FMT_UNIREC and req_format != "":
            URInputTmplt = pytrap.UnirecTemplate(req_format) # TRAP expects us to have predefined template for required set of fields
            rec = URInputTmplt

        stop = False
        while not stop:
            logger.info("Starting receiving")
            # *** Read data from input interface ***
            try:
                data = trap.recv()
            except pytrap.FormatMismatch:
                logger.error("Input data format mismatch in receiving from TRAP interface")
                break
            except pytrap.FormatChanged as e:
                # Get negotiated input data format
                (fmttype, fmtspec) = trap.getDataFmt(0)
                # If data type is UniRec, create UniRec template
                if fmttype == pytrap.FMT_UNIREC:
                    URInputTmplt = pytrap.UnirecTemplate(fmtspec)
                else:
                    URInputTmplt = None
                rec = URInputTmplt
                data = e.data
            except pytrap.Terminated:
                break

            # Check for "end-of-stream" record
            if len(data) <= 1:
                if args.trap:
                    # If we have output, send "end-of-stream" record and exit
                    trap.send(0, b"0")
                break

            # Assert that if UniRec input is required, input template is set
            assert(req_type != pytrap.FMT_UNIREC or URInputTmplt is not None)

            # Convert raw input data to UniRec object (if UniRec input is expected)
            if req_type == pytrap.FMT_UNIREC:
                rec.setData(data)
            elif req_type == pytrap.FMT_JSON:
                rec = json.loads(str(data))
            else: # TRAP_FMT_RAW
                rec = data

            # *** Convert input record to IDEA ***

            # Pass the input record to conversion function to create IDEA message
            idea = conv_func(rec, args)

            if idea is None:
                # Record can't be converted - skip it
                continue

            if args.name is not None:
                idea['Node'][0]['Name'] = args.name

            # Sanity check of timestamps
            if args.dontvalidate == False and not check_valid_timestamps(idea):
                print("Invalid timestamps in skipped message: {0}".format(json.dumps(idea)))
                continue

            # *** Send IDEA to outputs ***
            # Perform rule matching and action running on the idea message
            try:
                config.match(idea)
            except pytrap.Terminated:
                logger.error("PyTrap was terminated")
                break
            except DropMsg:
                logger.info("Message was dropped by Drop action.")
                continue
            except Exception as e:
                logger.error(str(e))
                break
    else:
        # DRY argument given, just print config and exit
        print(config)

    if not args.dry:
        if wardenclient:
            wardenclient.close()
        trap.finalize()
