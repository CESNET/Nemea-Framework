import yaml
from yaml.parser import ParserError


class Parser:
    config = None

    def __init__(self, config_path):
        # Parse given config file
        with open(config_path, 'r') as f:
            self.config = self.load(f.read())

    @classmethod
    def load(self, config):
        """Load and parse given string via yaml.load
        In case of YAML failing to parse return None.
        """
        cfg = None
        try:
            cfg = yaml.load(config, yaml.SafeLoader)
        except ParserError as e:
            print("Yaml file could not be parsed: " + str(e))
        return cfg

    def __str__(self):
        return yaml.dump(self.config)

    def __getitem__(self, key):
        return self.config[key]

    def __contains__(self, key):
        if self.config:
            return key in self.config

    def get(self, key, value=None):
        if self.config:
            return self.config.get(key, value)
        else:
            raise Exception("Configuration was not loaded.")
