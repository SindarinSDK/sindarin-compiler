"""Generated C/Rust controls for canonical native array field families."""
from textwrap import dedent
from native_array_field_contexts import combined_controls


def family_controls():
    leaves = [('int', '7'), ('long', '7l'), ('int32', '7i32'), ('uint', '7u'),
              ('uint32', '7u32'), ('byte', '7b'), ('bool', 'true'), ('char', "'a'"),
              ('float', '1.5f'), ('double', '1.5'), ('str', '"same"')]
    fields, initial, body = '', [], ''
    for i, (kind, value) in enumerate(leaves):
        fields += f'  values{i}: {kind}[]\n'
        initial.append(f'values{i}: {{{value}}}')
        body += (f'  f.values{i}.push({value})\n'
                 f'  var copied{i}: {kind}[] = f.values{i}\n'
                 f'  copied{i}.push({value})\n'
                 f'  println(f.values{i}.length==2 && copied{i}.length==3 && f.values{i}[0]=={value})\n')
    scalar = ('native struct Fields as ref =>\n' + fields + 'native fn make(): Fields =>\n'
              '  return Fields { ' + ', '.join(initial) + ' }\nfn main(): void =>\n'
              '  var f: Fields = make()\n' + body)
    fields, initial, body = '', [], ''
    for rank in range(2, 33):
        kind, value = 'int' + '[]' * rank, '{' * rank + '7' + '}' * rank
        fields += f'  rank{rank}: {kind}\n'
        initial.append(f'rank{rank}: {value}')
        body += (f'  println(f.rank{rank}.contains(f.rank{rank}[0]))\n'
                 f'  var copied{rank}: {kind} = f.rank{rank}\n'
                 f'  println(!f.rank{rank}.contains(copied{rank}[0]))\n')
    nested = ('native struct Fields as ref =>\n' + fields + 'native fn make(): Fields =>\n'
              '  return Fields { ' + ', '.join(initial) + ' }\nfn main(): void =>\n'
              '  var f: Fields = make()\n' + body)
    record = dedent('''\
        struct Row =>
          number: int
        native struct Fields as ref =>
          rows: Row[]
        native fn make(): Fields =>
          return Fields { rows: {Row {number:7}} }
        fn main(): void =>
          var f: Fields = make()
          f.rows.push(Row {number:9})
          var copied: Row[] = f.rows
          copied.push(Row {number:11})
          println(f.rows.length==2 && copied.length==3 && f.rows[1].number==9)
        ''')
    reference = (record.replace('struct Row =>', 'native struct Row as ref =>')
                 .replace('f.rows.push(Row {number:9})', 'f.rows.push(makeRow())')
                 .replace('copied.push(Row {number:11})', 'copied.push(makeRow())')
                 .replace('rows: {Row {number:7}}', 'rows: {makeRow()}')
                 .replace('fn main(): void =>', 'native fn makeRow(): Row =>\n  return Row {number:9}\nfn main(): void =>'))
    managed = (record.replace('  number: int\n', '  number: int\n  label: str\n')
               .replace('number:7}', 'number:7,label:"start"}')
               .replace('number:9}', 'number:9,label:"later"}')
               .replace('number:11}', 'number:11,label:"other"}')
               .replace('f.rows[1].number==9)', 'f.rows[1].number==9 && f.rows[1].label=="later")'))
    nested_reference = (reference.replace('rows: Row[]', 'rows: Row[][]')
                        .replace('rows: {makeRow()}', 'rows: {{makeRow()}}')
                        .replace('f.rows.push', 'f.rows[0].push')
                        .replace('copied: Row[]', 'copied: Row[][]')
                        .replace('copied.push', 'copied[0].push')
                        .replace('f.rows.length==2 && copied.length==3 && f.rows[1].number==9',
                                 'f.rows[0].length==2 && copied[0].length==3 && f.rows[0][1].number==9'))
    contexts, expected = combined_controls('native struct UdpReceiveResult as ref =>\n  _data: byte[]\n  _sender: str')
    return [('scalar-eleven', scalar, 11), ('nested-rank32', nested, 62),
            ('value-record', record, 1), ('managed-value-record', managed, 1),
            ('native-reference-record', reference, 1),
            ('nested-native-reference-record', nested_reference, 1),
            ('field-contexts', contexts, expected)]
