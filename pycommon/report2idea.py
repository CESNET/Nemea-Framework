import sys, os.path
import argparse
import json
import pytrap
import pymongo
from time import time, gmtime
from uuid import uuid4
from datetime import datetime
import logging

from reporter_config import Config
from reporter_config.actions import DropMsg
FORMAT="%(asctime)s %(module)s:%(filename)s:%(lineno)d:%(message)s"

logger = logging.getLogger(__name__)

def getRandomId():
    """Return unique ID of IDEA message. It is done by UUID in this implementation."""
    return str(uuid4())

def setAddr(idea_field, addr):
    """Set IP address into 'idea_field'.
This method automatically recognize IPv4 vs IPv6 and sets the correct information into the IDEA message.
Usage: setAddr(idea['Source'][0], rec.SRC_IP)"""

    if addr.isIPv4():
        idea_field['IP4'] = [str(addr)]
    else:
        idea_field['IP6'] = [str(addr)]

def getIDEAtime(unirecField = None):
    """Return timestamp in IDEA format (string).
    If unirecField is provided, it will convert it into correct format.
    Otherwise, current time is returned."""

    if unirecField:
        # Convert UnirecTime
        ts = unirecField.toDatetime()
        return ts.strftime('%Y-%m-%dT%H:%M:%SZ')
    else:
        g = gmtime()
        iso = '%04d-%02d-%02dT%02d:%02d:%02dZ' % g[0:6]
    return iso


# TODO: resolve argument parsing and help in Python modules
# Ideally it should all be done in Python using overloaded ArgParse

# TODO: allow setting library verbose mode

# Template of module description
desc_template = """
TRAP module, libtrap version: [TODO]
===========================================
Name: {name}
Inputs: 1
Outputs: 0 or 1 (depending on --trap parameter)
Description:
  {original_desc}Required format of input:
    {type}: "{fmt}"

  All '<something>2idea' modules convert reports from various detectors to Intrusion Detection Extensible Alert (IDEA) format. The IDEA messages may be send to any of the following outputs:
    - TRAP interface (--trap)
    - simple text file (--file)
    - collection in MongoDB database (--mongodb)
    - Warden3 server (--warden)
  It is possible to define more than one outputs - the messages will be send to all of them.
"""

DEFAULT_NODE_NAME = "undefined"

def Run(module_name, module_desc, req_type, req_format, conv_func, arg_parser = None):
    """ TODO doc
    """

    # *** Parse command-line arguments ***
    if arg_parser is None:
       arg_parser = argparse.ArgumentParser()
    arg_parser.formatter_class = argparse.RawDescriptionHelpFormatter

    # Set description
    arg_parser.description = str.format(desc_template,
        name=module_name,
        type={pytrap.FMT_RAW:'raw', pytrap.FMT_UNIREC:'UniRec', pytrap.FMT_JSON:'JSON'}.get(req_type,'???'),
        fmt=req_format,
        original_desc = module_desc+"\n\n  " if module_desc else "",
    )

    # Add arguments defining outputs
    # TRAP output
    arg_parser.add_argument('--trap', action='store_true',
                            help='Enable output via TRAP interface (JSON type with format id "IDEA"). Parameters are set using "-i" option as usual.')
    # Config file
    arg_parser.add_argument('-c', '--config', metavar="FILE", default="./config.yaml", type=str,
                            help='Specify YAML config file path which to load.')
    # Warden3 output
    arg_parser.add_argument('--warden', metavar="CONFIG_FILE",
                            help='Send IDEA messages to Warden server. Load configuration of Warden client from CONFIG_FILE.')

    # Other options
    arg_parser.add_argument('-n', '--name', metavar='NODE_NAME',
                            help='Name of the node, filled into "Node.Name" element of the IDEA message. Required if Warden output is used, recommended otherwise.')
    arg_parser.add_argument('-v', '--verbose', metavar='VERBOSE_LEVEL', default=3, type=int,
                            help="""Enable verbose mode (may be used by some modules, common part donesn't print anything).\nLevel 1 logs everything, level 5 only critical errors. Level 0 doesn't log.""")
    # TRAP parameters
    trap_args = arg_parser.add_argument_group('Common TRAP parameters')
    trap_args.add_argument('-i', metavar="IFC_SPEC", required=True,
                           help='TODO (ideally this section should be added by TRAP')
    # Parse arguments
    args = arg_parser.parse_args()

    # Set log level
    logging.basicConfig(level=(args.verbose*10), format=FORMAT)

    # Check if node name is set if Warden output is enabled
    if args.name is None:
        #if args.warden:
        #    sys.stderr.write(module_name+": Error: Node name must be specified if Warden output is used (set param --name).\n")
        #    exit(1)
        logger.warning("Node name is not specified.")

    # *** Initialize TRAP ***
    trap = pytrap.TrapCtx()
    logger.info("Trap arguments: %s", args.i)
    trap.init(["-i", args.i], 1, 1 if args.trap else 0)
    #trap.setVerboseLevel(3)
    #trap.registerDefaultSignalHandler()

    # Set required input format
    trap.setRequiredFmt(0, req_type, req_format)

    # Initialize configuration
    config = Config.Config(args.config, trap = trap)

    # *** Create output handles/clients/etc ***
    wardenclient = None

    if args.warden:
        import warden_client
        config = warden_client.read_cfg(args.warden)
        config['name'] = args.name
        wardenclient = warden_client.Client(**config)

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
            logger.error("Input data format mismatch in receiving from TRAP interface")#Required: "+str((req_type,req_format))+"\nReceived: "+str(trap.get_data_fmt(trap.IFC_INPUT, 0))+"\n")
            break
        except pytrap.FormatChanged as e:
            # TODO: This should be handled by trap.recv transparently
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
            # If we have output, send "end-of-stream" record and exit
            if args.trap:
                trap.send(0, b"0")
            break

        # Assert that if UniRec input is required, input template is set
        assert(req_type != pytrap.FMT_UNIREC or URInputTmplt is not None)

        # Convert raw input data to UniRec object (if UniRec input is expected)
        if req_type == pytrap.FMT_UNIREC:
            rec.setData(data)
        elif req_type == pytrap.FMT_JSON:
            rec = json.loads(data)
        else: # TRAP_FMT_RAW
            rec = data

        # *** Convert input record to IDEA ***

        # Pass the input record to conversion function to create IDEA message
        idea = conv_func(rec, args)

        if idea is None:
            # Record can't be converted - skip it
            logger.warning("Record can't be converted")
            continue

        if args.name is not None:
            idea['Node'][0]['Name'] = args.name

        # *** Send IDEA to outputs ***
        # Perform rule matching and action running on the idea message
        try:
            config.match(idea)
        except pytrap.Terminated:
            logger.error("PyTrap was terminated")
            break
        except DropMsg:
            logger.info("Message was dropped")
            continue
        except pymongo.errors as e:
            logger.error(str(e))
            break

        # Warden output
        if wardenclient:
            wardenclient.sendEvents([idea])


    if wardenclient:
        wardenclient.close()
    trap.finalize()
