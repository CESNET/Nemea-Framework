import pytrap
import json

c = pytrap.TrapCtx()
c.init(["-i", "f:/dev/stdout:w"], 0, 1)

a = json.dumps({"a": 123, "b": "aaa"})

c.setDataFmt(0, pytrap.FMT_JSON, "JSON")

c.send(bytearray(a, "utf-8"))

c.finalize()

print("\nFinished")

