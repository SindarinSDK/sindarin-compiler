"""Shared typed contracts for generated native package adapters."""
TYPES = {'int': ('long long', 'int64_t'), 'long': ('long long', 'int64_t'),
         'uint': ('uint64_t', 'uint64_t'), 'int32': ('int32_t', 'int32_t'),
         'uint32': ('uint32_t', 'uint32_t'), 'byte': ('unsigned char', 'uint8_t'),
         'char': ('char', 'uint8_t'), 'bool': ('bool', 'uint8_t'),
         'float': ('float', 'float'), 'double': ('double', 'double'),
         'string': ('char *', 'SnAbiValue *'), 'void': ('void', 'void'),
         'string_array': ('SnArray *', 'SnAbiValue *'),
         'byte_array': ('SnArray *', 'SnAbiValue *'),
         'native_array': ('SnArray *', 'SnAbiValue *'),
         'record': ('void *', 'SnAbiValue *')}


def kind(value):
    name = value['kind']
    if name == 'struct' and value.get('native_record') and value.get('is_native') and value.get('pass_self_by_ref'):
        return 'record'
    if name == 'array' and value.get('element_type', {}).get('kind') == 'string':
        return 'string_array'
    if name == 'array' and value.get('element_type', {}).get('kind') == 'byte':
        return 'byte_array'
    if name == 'array':
        native_array_shape(value)
        return 'native_array'
    if name not in TYPES: raise ValueError(f'package adapter ABI type is not implemented: {name}')
    return name


def raw_type(value):
    return value['native_record']['c_type'] + ' *' if kind(value) == 'record' else TYPES[kind(value)][0]


def native_array_shape(value):
    rank = 0
    while value.get('kind') == 'array':
        rank += 1
        value = value.get('element_type', {})
    leaves = {'int':1,'long':1,'int32':2,'uint':3,'uint32':4,'byte':5,'bool':6,
              'float':7,'double':8,'char':9,'string':10}
    if not 1 <= rank <= 32 or value.get('kind') not in leaves:
        raise ValueError('canonical array leaf/rank requires a scalar/string contract')
    return leaves[value['kind']], rank


def c_array_shape(value):
    leaf, rank = native_array_shape(value)
    return f'(SnAbiNativeArrayType){{{leaf}, {rank}}}'


def record_types(signatures):
    records = {}
    for signature in signatures:
        for value in [signature['return_type']] + [p['type'] for p in signature['params']]:
            if kind(value) == 'record':
                record = value['native_record']
                previous = records.get(record['identity'])
                if previous and previous != record:
                    raise ValueError('conflicting native record identity contracts')
                records[record['identity']] = record
    return list(records.values())


def validate(signature):
    binding = signature['binding']
    ownership, params = binding['ownership'], signature['params']
    result = kind(signature['return_type'])
    if result == 'string_array' and signature.get('abi') not in ('1.1', '1.5', '1.6', '1.7'):
        raise ValueError('managed string-array results require native ABI 1.1, 1.5 or 1.6')
    if result == 'byte_array' and signature.get('abi') not in ('1.6','1.7'):
        raise ValueError('native byte-array transport requires ABI 1.6')
    names = {p['name'] for p in params}
    if set(ownership['parameters']) != names:
        raise ValueError('package parameter ownership does not match the resolved declaration')
    if result == 'native_array' and signature.get('abi') != '1.7':
        raise ValueError('typed canonical array transport requires ABI 1.7')
    expected_result = ('owned', 'borrowed') if result == 'record' else ('owned',) if result in ('string', 'string_array', 'byte_array', 'native_array') else ('value',)
    if ownership['result'] not in expected_result:
        raise ValueError('package result ownership requires an implemented owned-string or plain-value contract')
    for p in params:
        if kind(p['type']) == 'string_array' and signature.get('abi') not in ('1.1', '1.5', '1.6', '1.7'):
            raise ValueError('managed string-array inputs require native ABI 1.1, 1.5 or 1.6')
        if kind(p['type']) == 'byte_array' and signature.get('abi') not in ('1.6','1.7'):
            raise ValueError('native byte-array transport requires ABI 1.6')
        if kind(p['type']) == 'void':
            raise ValueError('package parameter cannot have void ABI representation')
        if kind(p['type']) == 'native_array' and signature.get('abi') != '1.7':
            raise ValueError('typed canonical array transport requires ABI 1.7')
        if kind(p['type']) == 'record' and signature.get('abi') not in ('1.5', '1.6', '1.7'):
            raise ValueError('native record transport requires ABI 1.5 or 1.6')
        expected = 'borrowed' if kind(p['type']) in ('string', 'string_array', 'byte_array', 'native_array', 'record') else 'value'
        if ownership['parameters'][p['name']] != expected:
            raise ValueError('package input ownership requires an implemented borrowed-string or plain-value contract')
    if result == 'record' and signature.get('abi') not in ('1.5', '1.6', '1.7'):
        raise ValueError('native record transport requires ABI 1.5 or 1.6')
    if result == 'record' and ownership['result'] == 'borrowed':
        source = next((p for p in params if p['name'] == ownership.get('borrowed_from')),None)
        if not source or kind(source['type']) != 'record':
            raise ValueError('borrowed record result requires a borrowed record owner parameter')
