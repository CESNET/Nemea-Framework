class URWrapper:
    """Helper class for data access to UniRec fields."""

    def __init__(self, tmpl):
        """Constructor.
        
Args:
    tmpl (UnirecTemplate): Instance of UniRec template."""
        self._tmpl = tmpl
        self._urdict = tmpl.getFieldsDict()
        self._numfields = len(self._urdict)

    def setData(self, data):
        """Set UniRec message before accessing fields.

Args:
    data (bytes): UniRec message."""
        self._data = data

    def __iter__(self):
        for i in range(self._numfields):
            yield self._tmpl.get(i, self._data)

    def __getattr__(self, a):
        """Get value of the field using its name as attribute.

Args:
    a (str): Name of the field (e.g. SRC_IP)"""
        return self._tmpl.get(self._urdict[a], self._data)

    def __len__(self):
        """Return number of UniRec fields in the template."""
        return self._numfields

    def strRecord(self):
        return "\n".join(["{0} ({2})\t=\t{1}".format(i, self.__getattr__(i), self._urdict[i]) for i in self._urdict])

