from pytrap.helpers import *
from pytrap.pytrap import *

try:
    import pandas as pd
    pd.read_nemea = read_nemea
except ModuleNotFoundError:
    pass

