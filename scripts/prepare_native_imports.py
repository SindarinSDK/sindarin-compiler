#!/usr/bin/env python3
"""Generate consumer C ABI adapters from compiler-resolved package declarations."""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace
import re
import contextlib
import io
from build_native_package import build, build_go_graph, load_prebuilt, run

from native_contract import TYPES, kind, validate, raw_type, record_types, c_array_operation, array_record
from native_provider import record_includes, c_consumer as adapter



def native_link_options(flags):
    """Keep paired driver options intact across individual @link pragmas."""
    result = []
    iterator = iter(flags)
    for flag in iterator:
        if flag in ('-framework', '-weak_framework', '-Xlinker', '-L', '-F', '-l'):
            argument = next(iterator, None)
            if argument is None:
                raise ValueError('native linker option is missing its argument: ' + flag)
            if flag in ('-framework', '-weak_framework'):
                result.append('-Wl,' + flag + ',' + argument)
            elif flag == '-Xlinker':
                result.append('-Wl,' + argument)
            else:
                result.append(flag + argument)
        else:
            result.append(flag)
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contract', type=Path, required=True)
    parser.add_argument('--compiler', type=Path, required=True)
    args=parser.parse_args()
    request=json.loads(args.contract.read_text())
    source_lines=['#include <stdint.h>','#include <stdbool.h>','#include <stdlib.h>',
                  '#include <stdio.h>','#include <string.h>']
    sources,links,prototypes=[],[],[]
    record_headers=[]
    compiler=args.compiler.resolve()
    runtime=compiler.parent/'lib'/('clang' if __import__('os').name=='nt' else 'gcc')/'libsn_runtime_min.a'
    header=compiler.parent/'include/runtime/sn_abi.h'
    source_lines.append('#include '+json.dumps(str(header).replace('\\','/')))
    source_lines.append('#include '+json.dumps(str(header.with_name('sn_array.h')).replace('\\','/')))
    source_lines += ['static void sn_package_string_slot_release(void *p) { free(*(char **)p); }',
                     'static void sn_package_string_slot_copy(const void *s, void *d) { *(char **)d = *(char *const *)s ? strdup(*(char *const *)s) : NULL; }']
    plans=[]
    for package in request['packages']:
        plan=json.loads(run([compiler,'--native-plan',Path(package['manifest']).resolve(),
                             '--target',request['target'],'-O'+str(request['optimization']),
                             '--'+request['arithmetic']]))
        plans.append({'manifest':package['manifest'],'plan':plan})
    prebuilt_go=sum(any(u['language']=='GO' for u in p['plan']['native']['builds'])
                    for p in plans if 'assembly' in p['plan']['native'])
    source_go=sum(u['language']=='GO' for p in plans if 'assembly' not in p['plan']['native'] for u in p['plan']['native']['builds'])
    if prebuilt_go and (prebuilt_go>1 or source_go):
        raise ValueError('prebuilt Go runtime archives cannot be combined with other Go packages; rebuild one aggregate source graph')
    aggregate=source_go>1
    exports={}
    for package,planned in zip(request['packages'],plans):
        headers=record_includes(package['signatures'],Path(package['manifest']).resolve().parent)
        source_lines+=headers
        record_headers+=headers
        for record in planned['plan']['native'].get('types',[]):
            for role in ('create','retain','release','refs','copy'):
                if role not in record: continue
                symbol=record[role]
                previous=exports.get(symbol)
                if previous and previous!=package['manifest']:
                    raise ValueError(f"native record lifecycle symbol '{symbol}' is shared by packages '{previous}' and '{package['manifest']}'")
                exports[symbol]=package['manifest']
        for signature in package['signatures']:
            symbol=signature['binding']['symbol']
            previous=exports.get(symbol)
            if previous and previous!=package['manifest']:
                raise ValueError(f"native export symbol '{symbol}' is shared by packages '{previous}' and '{package['manifest']}'")
            exports[symbol]=package['manifest']
        # Validate ownership/representations before running a backing toolchain.
        adapters=[adapter(s,package['manifest']) for s in package['signatures']]
        for signature in package['signatures']:
            result=kind(signature['return_type'])
            parameters=', '.join(f'{raw_type(p["type"])} p{i}' for i,p in enumerate(signature['params'])) or 'void'
            prototypes.append(f'{raw_type(signature["return_type"])} {signature["adapter"]}({parameters});')
        build_args=SimpleNamespace(compiler=compiler,manifest=Path(package['manifest']),
                                  out_dir=args.contract.parent/'artifacts',target=request['target'],
                                  optimization=str(request['optimization']),arithmetic=request['arithmetic'],
                                  validated_plan=planned['plan'],skip_go=aggregate)
        if 'assembly' in planned['plan']['native']:
            metadata,assembly=load_prebuilt(planned['plan'],package['manifest'],compiler)
        else:
            output=io.StringIO()
            with contextlib.redirect_stdout(output): build(build_args)
            built=json.loads(output.getvalue())
            assembly=Path(built['assembly'])
            metadata=json.loads(assembly.read_text())
        for unit in metadata['units']:
            links.append(str(assembly.parent/unit['archive']))
            links += native_link_options(unit['native_link_flags'])
            links += ['-l'+name for name in unit['libraries']]
        source_lines+=adapters
    if aggregate:
        graph=build_go_graph([p for p in plans if 'assembly' not in p['plan']['native']],compiler,args.contract.parent/'go-graphs',
                             str(request['optimization']),request['arithmetic'])
        links.append(graph['archive'])
        links+=native_link_options(graph['native_link_flags'])
    links.append(str(runtime))
    source=args.contract.with_suffix('.c').resolve()
    source.write_text('\n\n'.join(source_lines)+'\n')
    sources.append(str(source))
    header_path=source.with_suffix('.h')
    header_path.write_text('#include <stdint.h>\n#include <stdbool.h>\n#include '+json.dumps(str(header.with_name('sn_array.h')).replace('\\','/'))+'\n'+'\n'.join(list(dict.fromkeys(record_headers))+prototypes)+'\n')
    Path(request['output']).write_text(json.dumps({'sources':sources,'includes':[str(header_path)],'links':list(dict.fromkeys(links))},indent=2)+'\n')


if __name__=='__main__':
    try: main()
    except (ValueError,OSError,KeyError) as error:
        raise SystemExit('error: native package import adapter: '+str(error))
