from struct import pack
from struct import unpack as origin_unpack


need_convert = False


def unpack(format, data):
    global need_convert

    if need_convert and isinstance(data, memoryview):
        data = data.tobytes()

    try:
        return origin_unpack(format, data)
    except:
        need_convert = True

        data = data.tobytes()
        return origin_unpack(format, data)