from .Action import Action

import json
import syslog
import logging as log

logger = log.getLogger(__name__)

class SyslogAction(Action):
    """Send IDEA message to syslog.
    The syslog action expects the following parameters in the config file:
        * identifier (e.g., `nemea`)
        * logoption - options, can be chained using | (e.g., `LOG_PID | LOG_CONS`)
        * facility (e.g., `LOG_DAEMON`)
        * priority (e.g., `LOG_ALERT`)
    To learn about all possible values, see man syslog.h (https://www.man7.org/linux/man-pages/man0/syslog.h.0p.html)
    """
    def __init__(self, action):
        super(type(self), self).__init__(actionId = action["id"], actionType = "syslog")

        a = action["syslog"]

        self.ident = a.get("identifier", "nemea")

        self.logoption = a.get("logoption", "LOG_PID")
        self.logoption_code = 0
        for opt in self.logoption.split("|"):
            try:
                self.logoption_code |= self.syslog_lookup_const(opt) 
            except KeyError:
                raise Exception(f"Syslog: unknown value {opt} in `logoption`.")
                
        self.facility = a.get("facility", "LOG_DAEMON")
        try:
            self.facility_code = self.syslog_lookup_const(self.facility)
        except KeyError:
            raise Exception(f"Syslog: unknown value {self.facility} in `facility`.")

        self.priority = a.get("priority", "LOG_ALERT")
        try:
            self.priority_code = self.syslog_lookup_const(self.priority)
        except KeyError:
            raise Exception(f"Syslog: unknown value {self.priority} in `priority`.")

        syslog.openlog(ident=self.ident, logoption=self.logoption_code, facility=self.facility_code)

    def syslog_lookup_const(self, const):
        """Return value of the string constant from syslog or raise KeyError exception."""
        retval = vars(syslog).get(const.strip(), None)
        if not retval:
            raise KeyError
        return retval

    def run(self, record):
        super(type(self), self).run(record)
        try:
            syslog.syslog(self.priority_code, json.dumps(record))
        except Exception as e:
            self.logger.error(e)

    def __str__(self):
        return f"Syslog: {self.ident} logoption: {self.logoption} ({self.logoption_code}) facility: {self.facility} ({self.facility_code}) priority: {self.priority} ({self.priority_code})\n"


