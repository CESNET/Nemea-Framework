from .Action import Action

import smtplib
from email.mime.text import MIMEText
import json
from idea.lite import Timestamp

class EmailAction(Action):
    """Action to send an email with IDEA record to a specified address

    Mail is send by localhost SMTP server. In the future this
    should be configurable in the config.

    Parameter "subject" may contain variables that are substituted by 
    corresponding field from IDEA:
      $category - Category (joined by ',' in case of multiple categories)
      $node - Name of the last item in Node array (i.e. the original detector)
      $src_ip - First IP address in Source, followed by "(...)" in case there 
                are more than one.
      $tgt_ip - The same as $src_ip, but with Target.
    """
    def __init__(self, action):
        super(type(self), self).__init__(actionId = action["id"], actionType = "email")

        a = action["email"]
        if "to" in a:
            to = a["to"]
            self.logger.info("To: {0}".format(to))
            self.addrsTo = [email.strip() for email in to.split(",")]
        else:
            raise Exception("Email action needs `to` parameter, check your YAML config.")

        if "subject" in a:
            self.subject = a["subject"]
        else:
            raise Exception("Email action needs `subject` parameter, check your YAML config.")

        self.addrFrom   = a.get("from", "nemea@localhost")
        self.smtpServer = a.get("server", "localhost")
        self.smtpPort   = a.get("port", 25)
        self.smtpTLS    = a.get("startTLS", False)
        self.smtpSSL    = a.get("forceSSL", False)
        self.smtpUser   = a.get("authuser", None)
        self.smtpPass   = a.get("authpass", None)
        self.smtpKey    = None
        self.smtpChain  = None
        key    = a.get("key", None)
        chain  = a.get("chain", None)

        if key:
            try:
                with open(key, "r") as f:
                    self.smtpKey = f.read()
            except Exception as e:
                self.smtpKey = None
                self.logger.error(e)
        if chain:
            try:
                with open(chain, "r") as f:
                    self.smtpChain = f.read()
            except Exception as e:
                self.smtpChain = None
                self.logger.error(e)

    def run(self, record):
        """Send the record via email

        Record is pretty printed and headers are set according to the config
        """
        super(type(self), self).run(record)

        # Set message body
        self.message = MIMEText(json.dumps(record, indent=4))

        # Set "Subject" header
        category = ','.join(record.get('Category', [])) or 'N/A'
        try:
            node = record['Node'][-1]['Name']
        except KeyError:
            node = 'N/A'
        src_ips = [ip for src in record.get('Source', []) for ip in src.get('IP4',[]) + src.get('IP6', [])]
        src_ip = src_ips[0] + (' (...)' if len(src_ips) > 1 else '') if src_ips else 'N/A'
        tgt_ips = [ip for tgt in record.get('Target', []) for ip in tgt.get('IP4',[]) + tgt.get('IP6', [])]
        tgt_ip = tgt_ips[0] + (' (...)' if len(tgt_ips) > 1 else '') if tgt_ips else 'N/A'
        # Get duration of attack (for rate computation below)
        starttime = record.get('EventTime') or record.get('WinStartTime')
        endtime = record.get('CeaseTime') or record.get('WinEndTime')
        if starttime and endtime:
            try:
                duration = (Timestamp(endtime) - Timestamp(starttime)).total_seconds()
            except ValueError:
                duration = None
        # byte_rate (in MiB/s)
        if duration and 'ByteCount' in record:
            byte_rate = '{0:.2f} Mb/s'.format(record['ByteCount'] * 8.0 / (duration*1024*1024))
        else:
            byte_rate = ''
        # flow_rate (in flow/s)
        if duration and 'FlowCount' in record:
            flow_rate = '{0:.2f} flow/s'.format(record['FlowCount'] / duration)
        else:
            flow_rate = ''

        self.message['Subject'] = self.subject\
            .replace('$category', category)\
            .replace('$node', node)\
            .replace('$src_ip', src_ip)\
            .replace('$tgt_ip', tgt_ip)\
            .replace('$byte_rate', byte_rate)\
            .replace('$flow_rate', flow_rate)

        # Set other headers
        self.message['From'] = self.addrFrom
        self.message['To'] = ",".join(self.addrsTo)

        # Send message
        self.logger.info("Mail From: {0} To: {1} Subject: {2}".format(self.addrFrom, self.addrsTo, self.message['Subject']))
        smtp = None
        try:
            if self.smtpSSL:
                smtp = smtplib.SMTP_SSL(host = self.smtpServer, port = self.smtpPort,
                                        keyfile = self.smtpKey, certfile = self.smtpChain)
            else:
                smtp = smtplib.SMTP(host = self.smtpServer, port = self.smtpPort)
                if self.smtpTLS:
                    smtp.starttls(keyfile = self.smtpKey, certfile = self.smtpChain)
            if self.smtpUser and self.smtpPass:
                smtp.login(self.smtpUser, self.smtpPass)
            smtp.sendmail(self.addrFrom, self.addrsTo, self.message.as_string())
        except Exception as e:
            self.logger.error(e)
        if smtp:
            smtp.quit()


    def __str__(self):
        f = []
        f.append("Server: " + self.smtpServer)
        f.append("Port: " + str(self.smtpPort))
        f.append("STARTTLS: " + ("True" if self.smtpTLS else "False"))
        f.append("SSL:" + ("True" if self.smtpSSL else "False"))
        if self.smtpUser:
            f.append("Username: " + self.smtpUser)
        if self.smtpPass:
            f.append("*PASSWORD*")
        if self.smtpKey:
            f.append("*KEYFILE*")
        if self.smtpChain:
            f.append("*CHAINFILE*")
        f.append("From: " + self.addrFrom)
        f.append("To: " + ",".join(self.addrsTo))
        f.append("Subject: " + self.subject)
        return ", ".join(f) + "\n"


