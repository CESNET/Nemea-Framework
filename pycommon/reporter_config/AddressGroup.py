import ipranges

import logging as log

logger = log.getLogger(__name__)

class AddressGroup:
    content = list()

    def __init__(self, addrGroup):
        self.content = list()
        if not isinstance(addrGroup, dict):
            raise Exception("AddressGroup must be a dictionary")

        if "file" in addrGroup:
            with open(addrGroup["file"], 'r') as f:
                for line in f:
                    line = line.strip('\n')
                    if not line:
                        continue
                    try:
                        self.content.append(ipranges.from_str(line))
                    except ValueError as e:
                        logger.error("IP address ({0}) could not be parsed.".format(line))
                        continue
        elif "list" in addrGroup:
            for ip in addrGroup["list"]:
                try:
                    self.content.append(ipranges.from_str(ip))
                except ValueError as e:
                    logger.error("IP address ({0}) could not be parsed.".format(ip))
                    continue
        else:
            raise Exception("Only 'file' or 'list' keys are supported.")

        self.id = addrGroup["id"]

    def __str__(self):
        return "ID: '" + self.id + "' IPs: " + str(self.content)

    def iplist(self):
        return repr([str(i) for i in self.content])

    def isPresent(self, ip):
        """
        Return True if `ip` is in the addressgroup.

        This method may raise ValueError if the `ip` is not a valid IPv4/6 address or subnet.

        :param str ip: IPv4 or IPv6 address or subnet
        :return: True if `ip` is present, False otherwise.
        """
        ip = ipranges.from_str(ip)
        return any((ip in rng) for rng in self.content)

