from pytrap.pytrap import *

def read_nemea(ifc_spec, nrows=-1, array=False):
    """
    Read `nrows` records from NEMEA TRAP interface given by `ifc_spec` and convert then into Pandas DataFrame.

    Args:
        ifc_spec (str): IFC specifier for TRAP input IFC, see https://nemea.liberouter.org/trap-ifcspec/
        nrows (int): Number of records, read until end of stream (zero size message) if -1.
        array (bool): Set output type to list of dictionary instead of pandas.DataFrame

    Returns:
        pandas.DataFrame or list of dictionary: DataFrame if array is False, otherwise, list of dictionary

    Raises:
        ModuleNotFoundError: When pandas is not installed.
    """
    c = pytrap.TrapCtx()
    c.init(["-i", ifc_spec], 1, 0)
    c.setRequiredFmt(0)
    rec = None
    l = list()
    while nrows != 0:
        try:
            data = c.recv()
        except pytrap.FormatChanged as e:
            fmttype, fmtspec = c.getDataFmt(0)
            rec = pytrap.UnirecTemplate(fmtspec)
            data = e.data
        if len(data) <= 1:
            break
        rec.setData(data)
        d = rec.getDict()
        if d:
            l.append(d)
        if nrows > 0:
            nrows = nrows - 1
    c.finalize()
    del(c)
    if array:
        return l
    else:
        import pandas as pd
        return pd.DataFrame(l)

try:
    import pandas as pd
    pd.read_nemea = read_nemea
except ModuleNotFoundError:
    pass


