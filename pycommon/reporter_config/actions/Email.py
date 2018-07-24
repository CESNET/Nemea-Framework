import os

from .Action import Action

from jinja2 import Environment, FileSystemLoader
import jinja2
import smtplib
from email.mime.text import MIMEText
import json
from idea.lite import Timestamp

class EmailAction(Action):
    """Action to send an email with IDEA record to a specified address

    Parameters for SMTP server are configurable in the config.

    Data are filled to body of messsage by template file. Path to template is
    configurable in config and is possible to use
    different templates for different messages.
    """
    DEFAULT_TEMPLATE_PATH = "/etc/nemea/email-templates/default.html"

    def __init__(self, action, smtp_conn):
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

        self.smtp_id    = smtp_conn.get("id", "localserver")
        self.smtpServer = smtp_conn.get("server", "localhost")
        self.smtpPort   = smtp_conn.get("port", 25)
        self.smtpUser   = smtp_conn.get("user", None)
        self.smtpPass   = smtp_conn.get("pass", None)
        self.smtpTLS    = smtp_conn.get("start_tls", False)
        self.smtpSSL    = smtp_conn.get("force_ssl", False)
        self.addrFrom   = a.get("from", "nemea@localhost")

        self.smtpKey    = None
        self.smtpChain  = None

        key    = smtp_conn.get("key", None)
        chain  = smtp_conn.get("chain", None)

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

        self.template_path = a.get("template", None)
        if self.template_path is None:
            print('Path to template is not specified, default template is used')
            self.template_path = EmailAction.DEFAULT_TEMPLATE_PATH
            if not os.path.exists(self.template_path):
                raise Exception("Loading default template file ({0}) failed. Check if path is valid.".format(self.template_path))


    def run(self, record):
        """Send the record via email

        Record is pretty printed and headers are set according to the config
        """
        super(type(self), self).run(record)

        # Use Jinja2 to fill template with data from record
        try:
            env = Environment(loader=FileSystemLoader(os.path.dirname(self.template_path)))
            body_template = env.get_template(os.path.basename(self.template_path))
        except jinja2.exceptions.TemplateNotFound:
            print('Path to template not found')
            raise

        # Set message body
        self.message = MIMEText(body_template.render(idea=record))

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

        subject_template = jinja2.Template(self.subject)

        self.message['Subject'] = subject_template.render({
            'category': category,
            'node': node,
            'src_ip': src_ip,
            'tgt_ip': tgt_ip,
            'byte_rate': byte_rate,
            'flow_rate': flow_rate,
        }, idea = record)

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
        f.append("Used template: {}".format(self.template_path))
        f.append("Smtp connection id: {}".format(self.smtp_id))
        return ", ".join(f) + "\n"


