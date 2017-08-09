from .Action import Action

import smtplib
from email.mime.text import MIMEText
import json

class EmailAction(Action):
    """Action to send an email with IDEA record to a specified address

    Mail is send by localhost SMTP server. In the future this
    should be configurable in the config
    """
    def __init__(self, action):
        super(type(self), self).__init__(actionId = action["id"], actionType = "email")

        if "to" in a:
            to = a["to"]
            self.addrsTo = [email.strip() for email in to.split(",")]
        else:
            raise Exception("Email action needs `to` parameter, check your YAML config.")

        if "subject" in a:
            self.subject = a["subject"]
        else:
            raise Exception("Email action needs `subject` parameter, check your YAML config.")

        self.addrFrom   = a["from"]     if "from"     in a else "nemea@localhost"
        self.smtpServer = a["server"]   if "server"   in a else "localhost"
        self.smtpPort   = a["port"]     if "port"     in a else 25
        self.smtpTLS    = a["startTLS"] if "startTLS" in a else False
        self.smtpSSL    = a["forceSSL"] if "forceSSL" in a else False
        self.smtpUser   = a["authuser"] if "authuser" in a else None
        self.smtpPass   = a["authpass"] if "authpass" in a else None
        key    = a["key"]   if "key"   in a else None
        chain  = a["chain"] if "chain" in a else None

        try:
            with open(key, "r") as f:
                self.smtpKey = f.read()
        except Exception:
            self.smtpKey = None
        try:
            with open(chain, "r") as f:
                self.smtpChain = f.read()
        except Exception:
            self.smtpChain = None

    def run(self, record):
        """Send the record via email

        Record is pretty printed and headers are set according to the config
        """
        self.messsage = MIMEText(json.dumps(record, indent=4))
        self.__setHeaders()

        smtp = None
        try:
            if self.smtpSSL:
                smtp = smtplib.SMTP_SSL(host = self.smtpServer, port = self.smtpPort,
                                        keyfile = self.smtpKey, certfile = self.smtpChain)
            else:
                smtp = smtplib.SMTP(host = self.smtpServer, port = self.smtpPort)
                if self.smtpTLS:
                    smtp.starttls(keyfile = self.smtp, certfile = self.smtpChain)
            if self.smtpUser and self.smtpPass:
                smtp.login(self.smtpUser, self.smtpPass)

            smtp.sendmail(self.addrFrom, self.addrsTo, self.message.as_string())
        except Exception as e:
            self.logger.error(e)
        if smtp:
            smtp.quit()

    def __setHeaders(self):
        """Set email headers to message

        This should only be used during the run method
        """
        self.message['Subject'] = self.subject
        self.message['From'] = self.addrFrom
        self.message['To'] = self.addrsTo

