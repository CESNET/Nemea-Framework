import pytrap

def read_nemea(ifc_spec, nrows=-1, array=False):
    """
    Read `nrows` records from NEMEA TRAP interface given by `ifc_spec` and convert then into Pandas DataFrame.

    Example:
        >>> import pytrap
        >>> pytrap.read_nemea("f:/tmp/testunirechelper", array=True)
        [{'SRC_IP': UnirecIPAddr('10.0.0.255'), 'SRC_PORT': 0}, {'SRC_IP': UnirecIPAddr('10.0.0.255'), 'SRC_PORT': 1}, ...]

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

class PytrapHelperStore():
    """
    Easy to use helper for pytrap context with one OUTPUT interface.
    It provides data_start() to prepare everything, store_record() to send
    prepared UniRec record, data_stop() to finalize everything.

    The class contains UnirecTemplate accessible via `rec`

    Example 1:

    >>> import pytrap
    >>> with pytrap.PytrapHelperStore("f:/tmp/testunirechelper:w", "ipaddr SRC_IP,uint16 SRC_PORT") as s:
    ...     for i in range(10):
    ...         s.rec.setFromDict({"SRC_IP": "10.0.0.1", "SRC_PORT": i})
    ...         s.store_record()

    Example 2:

    >>> import pytrap
    >>> s = pytrap.PytrapHelperStore("f:/tmp/testunirechelper:w", "ipaddr SRC_IP,uint16 SRC_PORT")
    >>> rec = s.data_start()
    >>> for i in range(10):
    ...     rec.setFromDict({"SRC_IP": "10.0.0.255", "SRC_PORT": i})
    ...     s.store_record()
    >>> s.data_stop()

    """
    def __init__(self, ifcspec, fmtspec):
        """Create new instance of pytrap context with one output interface to send/store data.

        Args:
            ifcspec (str): TRAP specifier for output interface, see https://nemea.liberouter.org/trap-ifcspec/
            fmtspec (str): UniRec template specifier
        """
        self.ifcspec = ifcspec
        self.fmtspec = fmtspec

    def __enter__(self):
        self.data_start()
        return self

    def __exit__(self, exception_type, exception_value, exception_traceback):
        self.data_stop()

    def data_start(self):
        """Start storing the test records into self.filename - initialization."""
        fmttype = pytrap.FMT_UNIREC
        self.rec = pytrap.UnirecTemplate(self.fmtspec)
        self.rec.createMessage(0)

        self.trap = pytrap.TrapCtx()
        self.trap.init(["-i", self.ifcspec], 0, 1)
        self.trap.setDataFmt(0, fmttype, self.fmtspec)
        return self.rec

    def store_record(self):
        """Store one UniRec record that was filled outside."""
        if self.trap:
            self.trap.send(self.rec.getData())

    def data_stop(self):
        """Finish storing test data."""
        if self.trap:
            self.trap.finalize()

class PytrapHelperLoad():
    """
    Easy to use helper for pytrap context with one INPUT interface.
    It provides data_start() to prepare everything, load_record() to receive
    and return one UniRec record inside UnirecTemplate, data_stop() to finalize
    everything.

    Example 1:

    >>> import pytrap
    >>> with pytrap.PytrapHelperLoad("f:/tmp/testunirechelper") as s:
    ...     rec = s.load_record()
    ...     while rec:
    ...         # Do something with record:
    ...         # print(rec.strRecord())
    ...         #
    ...         # Load the next record:
    ...         rec = s.load_record()

    Example 2:

    >>> import pytrap
    >>> s = pytrap.PytrapHelperLoad("f:/tmp/testunirechelper")
    >>> s.data_start()
    >>> rec = s.load_record()
    >>> while rec:
    ...    # Do something with record:
    ...    # print(rec.strRecord())
    ...    #
    ...    # Load the next record:
    ...    rec = s.load_record()
    >>> s.data_stop()

    """
    def __init__(self, ifcspec):
        self.ifcspec = ifcspec

    def __enter__(self):
        self.data_start()
        return self

    def __exit__(self, exception_type, exception_value, exception_traceback):
        self.data_stop()

    def data_start(self):
        """Start evaluation of the stored data - initialization."""
        self.trap = pytrap.TrapCtx()
        self.trap.init(["-i", self.ifcspec], 1, 0)
        self.trap.setRequiredFmt(0, pytrap.FMT_UNIREC)

    def load_record(self):
        """Load one UniRec record and return UnirecTemplate if the record was
        received. Otherwise, return None when no more data."""
        try:
            data = self.trap.recv()
        except pytrap.FormatChanged as err:
            fmttype, fmtspec = self.trap.getDataFmt(0)
            self.rec = pytrap.UnirecTemplate(fmtspec)
            data = err.data
        if len(data) <= 1:
            # empty message - do not process it!!!
            return None

        self.rec.setData(data)
        return self.rec

    def data_stop(self):
        """Finish evaluation."""
        if self.trap:
            self.trap.finalize()

if __name__ == "__main__":
    import doctest
    doctest.testmod()

