import netaddr

class AddressGroup:
    content = list()

    def __init__(self, addrGroup):
        self.content = list()
        if not isinstance(addrGroup, dict):
            raise Exception("AddressGroup must be a dictionary")

        if "file" in addrGroup:
            with open(addrGroup["file"], 'r') as f:
                for line in f:
                    try:
                        self.content.append(netaddr.IPNetwork(line.strip('\n')))
                    except netaddr.core.AddrFormatError as e:
                        # TODO: Should log parsing IP error
                        continue
        else:
            for ip in addrGroup["list"]:
                try:
                    self.content.append(netaddr.IPNetwork(ip))
                except netaddr.core.AddrFormatError as e:
                    # TODO: Should log parsing IP error
                    continue

        self.id = addrGroup["id"]

    def __str__(self):
        return "ID: '" + self.id + "' IPs: " + str(self.content)

    def iplist(self):
        l = list()

        for i in self.content:
            if i.size > 1:
                l.append(str(i))
            else:
                l.append(str(i.ip))

        return repr(l)

    def isPresent(self, ip):
        try:
            pIp = netaddr.IPAddress(ip)
        except ValueError:
            pIp = netaddr.IPNetwork(ip)

        for lIp in self.content:
            if pIp in lIp:
                return True
        return False

