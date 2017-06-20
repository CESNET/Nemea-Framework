import yaml

class Parser:
	config = None

	def __init__(self, config):
		self.config = self.load(config)

	@classmethod
	def load(self, config):
		"""Load and parse given string via yaml.load
		In case of YAML failing to parse return None.
		"""
		try:
			return yaml.load(config)
		except Exception as e:
			print(e)
			raise Exception("Error while parsing config file")

	def __str__(self):
		return yaml.dump(self.config)

	def __getitem__(self, key):
		return self.config[key]

	def __contains__(self, key):
		return key in self.config
