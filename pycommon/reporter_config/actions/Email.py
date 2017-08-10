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

        a = action["email"]
        if "to" in a:
            to = a["to"]
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
        self.message = MIMEText(json.dumps(record, indent=4))
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

            self.logger.info("Mail From: {0} To: {1} Subject: {2}".format(self.addrFrom, self.addrsTo, self.subject))
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

