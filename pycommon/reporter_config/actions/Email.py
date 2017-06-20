from .Action import Action

import smtplib
from email.mime.text import MIMEText
import json

__author__ = "Petr Stehlik"

class EmailAction(Action):
	"""Action to send an email with IDEA record to a specified address

	Mail is send by localhost SMTP server. In the future this
	should be configurable in the config
	"""

	# Cannot use from since it is a keyword
	__from = "nemea@localhost"
	msg = None

	def __init__(self, action):
		self.actionId = action["id"]
		self.to = action["email"]["to"]
		self.subject = action["email"]["subject"]

		self.smtp = smtplib.SMTP('localhost')

	def run(self, record):
		"""Send the record via email

		Record is pretty printed and headers are set according to the config
		"""
		self.msg = MIMEText(json.dumps(record, indent=4))
		self.__setHeaders()
		self.smtp.sendmail(self.__from, [self.to], self.msg.as_string())

	def __setHeaders(self):
		"""Set email headers to message

		This should only be used during the run method
		"""
		self.msg['Subject'] = self.subject
		self.msg['From'] = self.__from
		self.msg['To'] = self.to

	def __del__(self):
		"""Close connection to SMTP server
		"""
		self.smtp.quit()
