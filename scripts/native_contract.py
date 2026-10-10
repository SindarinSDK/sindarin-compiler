"""Shared typed contracts for generated native package adapters."""
TYPES = {'int': ('long long', 'int64_t'), 'long': ('long long', 'int64_t'),
         'uint': ('uint64_t', 'uint64_t'), 'int32': ('int32_t', 'int32_t'),
         'uint32': ('uint32_t', 'uint32_t'), 'byte': ('unsigned char', 'uint8_t'),
         'char': ('char', 'uint8_t'), 'bool': ('bool', 'uint8_t'),
         'float': ('float', 'float'), 'double': ('double', 'double'),
         'string': ('char *', 'SnAbiValue *'), 'void': ('void', 'void'),
         'string_array': ('SnArray *', 'SnAbiValue *')}


def kind(value):
    name = value['kind']
    if name == 'array' and value.get('element_type', {}).get('kind') == 'string':
        return 'string_array'
    if name not in TYPES: raise ValueError(f'package adapter ABI type is not implemented: {name}')
    return name


def validate(signature):
    binding = signature['binding']
    ownership, params = binding['ownership'], signature['params']
    result = kind(signature['return_type'])
    if result == 'string_array' and signature.get('abi') not in ('1.1', '1.5'):
        raise ValueError('managed string-array results require native ABI 1.1 or 1.5')
    names = {p['name'] for p in params}
    if set(ownership['parameters']) != names:
        raise ValueError('package parameter ownership does not match the resolved declaration')
    if ownership['result'] != ('owned' if result in ('string', 'string_array') else 'value'):
        raise ValueError('package result ownership requires an implemented owned-string or plain-value contract')
    for p in params:
        if kind(p['type']) == 'string_array' and signature.get('abi') not in ('1.1', '1.5'):
            raise ValueError('managed string-array inputs require native ABI 1.1 or 1.5')
        if kind(p['type']) == 'void':
            raise ValueError('package parameter cannot have void ABI representation')
        expected = 'borrowed' if kind(p['type']) in ('string', 'string_array') else 'value'
        if ownership['parameters'][p['name']] != expected:
            raise ValueError('package input ownership requires an implemented borrowed-string or plain-value contract')
