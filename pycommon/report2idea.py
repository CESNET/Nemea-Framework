import sys, os.path
import argparse
import json
import pytrap
from time import time, gmtime
from uuid import uuid4
from datetime import datetime

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
    # File output
    arg_parser.add_argument('--file', metavar="FILE", type=str,
                            help='Enable output to file (each IDEA message printed to new line in JSON format). Set to "-" to use standard output.')
    arg_parser.add_argument('--file-indent', metavar="N", type=int,
                            help='Pretty-format JSON in output file using N spaces for indentation.')
    arg_parser.add_argument('--file-append', action='store_true',
                            help='Append to file instead of overwrite.')
    # MongoDB output
    arg_parser.add_argument('--mongodb', metavar="DBNAME",
                            help='Enable output to MongoDB. Connect to database named DBNAME.')
    arg_parser.add_argument('--mongodb-coll', metavar="COLL", default='alerts',
                            help='Put IDEA messages into collection named COLL (default: "alerts").')
    arg_parser.add_argument('--mongodb-host', metavar="HOSTNAME", default='localhost',
                            help='Connect to MongoDB running on HOSTNAME (default: "localhost").')
    arg_parser.add_argument('--mongodb-port', metavar="PORT", type=int, default=27017,
                            help='Connect to MongoDB running on port number PORT (default: 27017).')
    # Warden3 output
    arg_parser.add_argument('--warden', metavar="CONFIG_FILE",
                            help='Send IDEA messages to Warden server. Load configuration of Warden client from CONFIG_FILE.')

    # Other options
    arg_parser.add_argument('-n', '--name', metavar='NODE_NAME',
                            help='Name of the node, filled into "Node.Name" element of the IDEA message. Required if Warden output is used, recommended otherwise.')
    arg_parser.add_argument('--test', action='store_true',
                            help='Add "Test" to "Category" before sending a message to output(s).')
    arg_parser.add_argument('-v', '--verbose', action='store_true',
                            help="Enable verbose mode (may be used by some modules, common part donesn't print anything")
    arg_parser.add_argument('--srcwhitelist-file', metavar="FILE", type=str,
                            help="File with addresses/subnets in format: <ip address>/<mask>,<data>\\n \n where /<mask>,<data> is optional, <data> is a user-specific optional content. Whitelist is applied to SRC_IP field. If SRC_IP from the alert is on whitelist, the alert IS NOT reported.")
    arg_parser.add_argument('--dstwhitelist-file', metavar="FILE", type=str,
                            help="File with addresses/subnets, whitelist is applied on DST_IP, see --srcwhitelist-file help.")
    # TRAP parameters
    trap_args = arg_parser.add_argument_group('Common TRAP parameters')
    trap_args.add_argument('-i', metavar="IFC_SPEC", required=True,
                           help='TODO (ideally this section should be added by TRAP')
    # Parse arguments
    args = arg_parser.parse_args()

    # Check if at least one output is enabled
    if not (args.file or args.trap or args.mongodb or args.warden):
        sys.stderr.write(module_name+": Error: At least one output must be selected\n")
        exit(1)

    # Check if node name is set if Warden output is enabled
    if args.name is None:
        if args.warden:
            sys.stderr.write(module_name+": Error: Node name must be specified if Warden output is used (set param --name).\n")
            exit(1)
        else:
            sys.stderr.write(module_name+": Warning: Node name is not specified.\n")

    # *** Initialize TRAP ***
    trap = pytrap.TrapCtx()
    trap.init(["-i", args.i], 1, 1 if args.trap else 0)
    #trap.setVerboseLevel(3)
    #trap.registerDefaultSignalHandler()

    # Set required input format
    trap.setRequiredFmt(0, req_type, req_format)

    # If TRAP output is enabled, set output format (JSON, format id "IDEA")
    if args.trap:
        trap.setDataFmt(0, pytrap.FMT_JSON, "IDEA")

    # *** Create output handles/clients/etc ***
    filehandle = None
    mongoclient = None
    mongocoll = None
    wardenclient = None

    if args.file:
        if args.file == '-':
            filehandle = sys.stdout
        else:
            filehandle = open(args.file, "a" if args.file_append else "w")

    if args.mongodb:
        import pymongo
        mongoclient = pymongo.MongoClient(args.mongodb_host, args.mongodb_port)
        mongocoll = mongoclient[args.mongodb][args.mongodb_coll]

    if args.warden:
        import warden_client
        config = warden_client.read_cfg(args.warden)
        config['name'] = args.name
        wardenclient = warden_client.Client(**config)

    # Check if a whitelist is set, parse the file and prepare context for binary search
    import ip_prefix_search
    if args.srcwhitelist_file:
        if 'ipaddr SRC_IP' in req_format.split(","):
            srcwhitelist = ip_prefix_search.IPPSContext.fromFile(args.srcwhitelist_file)
    else:
        srcwhitelist = None

    if args.dstwhitelist_file:
        if 'ipaddr DST_IP' in req_format.split(","):
            dstwhitelist = ip_prefix_search.IPPSContext.fromFile(args.dstwhitelist_file)
    else:
        dstwhitelist = None


    # *** Main loop ***
    URInputTmplt = None
    if req_type == pytrap.FMT_UNIREC and req_format != "":
        URInputTmplt = pytrap.UnirecTemplate(req_format) # TRAP expects us to have predefined template for required set of fields
        rec = URInputTmplt

    stop = False
    while not stop:
        # *** Read data from input interface ***
        try:
            data = trap.recv()
        except pytrap.FormatMismatch:
            sys.stderr.write(module_name+": Error: input data format mismatch\n")#Required: "+str((req_type,req_format))+"\nReceived: "+str(trap.get_data_fmt(trap.IFC_INPUT, 0))+"\n")
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

        # Check whitelists
        if srcwhitelist and srcwhitelist.ip_search(rec.SRC_IP):
             continue

        if dstwhitelist and dstwhitelist.ip_search(rec.DST_IP):
             continue

        # *** Convert input record to IDEA ***

        # Pass the input record to conversion function to create IDEA message
        idea = conv_func(rec, args)

        if idea is None:
            continue # Record can't be converted - skip it (notice should be printed by the conv function)

        if args.name is not None:
            idea['Node'][0]['Name'] = args.name

        if args.test:
            idea['Category'].append('Test')

        # *** Send IDEA to outputs ***

        # File output
        if filehandle:
            filehandle.write(json.dumps(idea, indent=args.file_indent)+'\n')

        # TRAP output
        if args.trap:
            try:
                trap.send(0, json.dumps(idea))
            except pytrap.Terminated:
                # don't exit immediately, first finish sending to other outputs
                stop = True

        # MongoDB output
        if mongocoll:
            # We need to change IDEA message here, but we may need it unchanged
            # later -> copy it (shallow copy is sufficient)
            idea2 = idea.copy()
            # Convert timestamps from string to Date format
            idea2['DetectTime'] = datetime.strptime(idea2['DetectTime'], "%Y-%m-%dT%H:%M:%SZ")
            for i in [ 'CreateTime', 'EventTime', 'CeaseTime' ]:
                if idea2.has_key(i):
                    idea2[i] = datetime.strptime(idea2[i], "%Y-%m-%dT%H:%M:%SZ")

            try:
                mongocoll.insert(idea2)
            except pymongo.errors.AutoReconnect:
                sys.stderr.write(module_name+": Error: MongoDB connection failure.\n")
                stop = True

        # Warden output
        if wardenclient:
            wardenclient.sendEvents([idea])


    # *** Cleanup ***
    if filehandle and filehandle != sys.stdout:
        filehandle.close()
    if mongoclient:
        mongoclient.close()
    if wardenclient:
        wardenclient.close()
    trap.finalize()
