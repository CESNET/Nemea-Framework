import yaml
from yaml.parser import ParserError
from yaml.scanner import ScannerError


class Parser:
    """Parse configuration file in Yaml format."""
    config = None

    def __init__(self, config_path):
        # Parse given config file
        with open(config_path, 'r') as f:
            self.config = self.load(f.read())

    @classmethod
    def load(cls, config):
        """Load and parse given string via yaml.load
        In case of YAML failing to parse return None.

        Raises SyntaxError
        """
        cfg = None
        try:
            cfg = yaml.load(config, yaml.SafeLoader)
        except (ParserError, ScannerError) as e:
            raise SyntaxError("Yaml file could not be parsed: %s" % str(e))

        return cfg

    def __str__(self):
        return yaml.dump(self.config)

    def __getitem__(self, key):
        return self.config[key]

    def __contains__(self, key):
        if self.config:
            return key in self.config
        return False

    def get(self, key, value=None):
        """Get value of the key from configuration."""
        if self.config:
            return self.config.get(key, value)
        raise Exception("Configuration was not loaded.")
