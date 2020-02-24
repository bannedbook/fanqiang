from weakref import ref


class GcWeakrefs(object):
    def __init__(self, ffi):
        self.ffi = ffi
        self.data = {}

    def build(self, cdata, destructor):
        # make a new cdata of the same type as the original one
        new_cdata = self.ffi.cast(self.ffi._backend.typeof(cdata), cdata)
        #
        def remove(key):
            # careful, this function is not protected by any lock
            old_key = self.data.pop(index)
            assert old_key is key
            destructor(cdata)
        #
        key = ref(new_cdata, remove)
        index = object()
        self.data[index] = key
        return new_cdata
