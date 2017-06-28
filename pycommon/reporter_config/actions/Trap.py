from .Action import Action

import pytrap
import logging
import json

logger = logging.getLogger(__name__)

class TrapAction(Action):
	def __init__(self, action, trap = None):
		self.actionId = action["id"]
		self.actionType = "trap"

		if trap == None:
			# Initialize TRAP interface
			self.trap = pytrap.TrapCtx()

			# Only output interface is needed
			self.trap.init(['-i', action["trap"]["config"]], 0, 1)

			# We must set output format to JSON with ID IDEA
			self.trap.setDataFmt(0, pytrap.FMT_JSON, "IDEA")
		else:
			self.trap = trap

	def run(self, record):
		self.trap.send(json.dumps(record).encode('utf8'), 0)

	def __del__(self):
		pass

