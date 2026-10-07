#!/usr/bin/env python3
"""Check frozen native callback C/Rust ABI, identity, alias and ownership contracts and complete mode coverage."""
import hashlib
import json
import os
from pathlib import Path
import sys

ORACLES = {'tests/integration/test_inline_pointer_passing.sn': b'Test 1 passed: mock_free(mock_alloc(100)'
                                                     b')\nTest 2 passed: use_resource(get_resour'
                                                     b'ce())\nTest 3 passed: use_resource(transf'
                                                     b'orm_pointer(get_resource()))\nTest 4 pass'
                                                     b'ed: combine_resources with two inline get_re'
                                                     b'source() calls\nTest 5 passed: mock_free('
                                                     b'nil)\nAll inline pointer passing tests pa'
                                                     b'ssed!\n',
 'tests/integration/test_interop_edge_cases.sn': b'=== Interop Edge Case Tests ===\n\nTest 1: Nil'
                                                 b' pointer comparisons\n  ptr == nil: PASS\n  pt'
                                                 b'r != nil when nil: PASS\n  Function test_nil_'
                                                 b'equality: PASS\n  Function test_nil_inequalit'
                                                 b'y: PASS\n\nTest 2: *char as val when NULL\n  Po'
                                                 b'inter is nil: PASS\n  Safe unwrap returns gua'
                                                 b'rd value: PASS\n\nTest 3: Mixed pointer and pr'
                                                 b'imitive parameters\n  Nil pointer detection: '
                                                 b'PASS\n  Mixed with nil and true flag: PASS\n\nT'
                                                 b'est 4: Callback with pointer parameters\n  Ca'
                                                 b'llback received nil: PASS\n  Callback !nil ch'
                                                 b'eck with nil: PASS\n\nTest 5: Pointer-to-point'
                                                 b'er nil handling\n  **int == nil: PASS\n  Funct'
                                                 b'ion with **int nil: PASS\n\n=== All edge case '
                                                 b'tests completed! ===\n',
 'tests/rust-native/scalar_native_callback_array_char_alias.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_char_clear_iteration.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_char_foreign.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_char_hygiene.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_char_methods.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_char_set.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_char_values.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_nested_alias.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_nested_element_store.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_nested_inner_mutation.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_nested_string_values.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_nested_values.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_record_field_mutation.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_record_pod_values.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_record_values.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_alias.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_captured_alias.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_captured_private_snapshot.sn': b'true\ntru'
                                                                                       b'e\n',
 'tests/rust-native/scalar_native_callback_array_string_captured_repeat.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_cell_alias.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_cell_nested_mutation.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_cell_reentry.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_cell_write.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_field_alias.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_foreign.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_hygiene.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_methods.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_native_alias.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_native_write.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_nil_elements.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_set.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_store_order.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_temporary_alias.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_temporary_scope.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_temporary_statement.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_array_string_values.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_char_bridge.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_char_escaped_pointer.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_char_field_c_copy.sn': b'true\ntrue\ntrue\n',
 'tests/rust-native/scalar_native_callback_char_retained_field.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_field_reentry.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_foreign_repeat.sn': b'true\ntrue\ntrue\n',
 'tests/rust-native/scalar_native_callback_identity.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_managed_record_escape_snapshot.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_managed_record_snapshot_escape.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_managed_record_snapshot_fields.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_managed_record_snapshot_forwarded.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_managed_record_snapshot_nested.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_native_nil_result.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_native_produced.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_native_return.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_nil_roundtrip.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_optional_named.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_optional_recursive.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_parallel_credits.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_retained.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_retained_identity.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_scalar_write_reentry.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_string_effect_order.sn': b'true\ntrue\ntrue\n',
 'tests/rust-native/scalar_native_callback_string_invoke.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_string_native_mutate.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_string_produced.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_string_retained_input.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_thread_foreign.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_thread_invoke.sn': b'true\ntrue\n',
 'tests/rust-native/scalar_native_callback_thread_retained_identity.sn': b'true\ntrue\n'}
SOURCE_SHA256 = {'tests/integration/test_inline_pointer_passing.sn': '9158155543ff08e270794e0f96c68a7a97e6e681f8a68601eee6d5c2fe4ce1a4',
 'tests/integration/test_interop_edge_cases.sn': 'fbae43e68034b6963e35331443f7087202620293d836a34bde0ddc02b04e3541',
 'tests/rust-native/scalar_native_callback_array_char_alias.sn': '7c3abe13ca8da9caa062afff724486d54dd238cbcda90c0c777fdd634923ce86',
 'tests/rust-native/scalar_native_callback_array_char_clear_iteration.sn': '88028c48c75fbc09b7e95f204139219e31f7f3b2b59c03722771d8617db32adb',
 'tests/rust-native/scalar_native_callback_array_char_foreign.sn': '9b39069577ff26f85150722d713f5519902974a309513f1656d313f1b5fd7cf7',
 'tests/rust-native/scalar_native_callback_array_char_hygiene.sn': '84f7389c9183eeb4b0d37865a981250848f37ecb6155e1e8835eb45646f8871b',
 'tests/rust-native/scalar_native_callback_array_char_methods.sn': '4468ceb2093e90897258cb7f19f1b7c6ec971d859513c54f8845afe1f493fa96',
 'tests/rust-native/scalar_native_callback_array_char_set.sn': '41438f7737589ca5e329d06f2f3788a7c44425a267c3402c8250507d7c95cf44',
 'tests/rust-native/scalar_native_callback_array_char_values.sn': '5010952809f01f3fae1fc7d669b9804f552d1f9432b8eeef3b5ca59aeae1e378',
 'tests/rust-native/scalar_native_callback_array_nested_alias.sn': '7a877eb66e01fb090c303bec6353a3ef861af0419f5e11cf25b8800576fb0f2d',
 'tests/rust-native/scalar_native_callback_array_nested_element_store.sn': '6bf1e120dcdabd725961425b95ad68f3da110b1513cb3e57cac86a7e9b5cf930',
 'tests/rust-native/scalar_native_callback_array_nested_inner_mutation.sn': '8be35d7dc0f841ca5c93ddbedadeec0debe37e379f805369255cc944ffc4dc43',
 'tests/rust-native/scalar_native_callback_array_nested_string_values.sn': '3e8bbcf3015ecf4e2e38ded573854f659b349f442534c14049bf15581c29b29c',
 'tests/rust-native/scalar_native_callback_array_nested_values.sn': '3cbd8ab192a278124f25c1053b9058ceaaa5d306daad62155fe5a82758e7b252',
 'tests/rust-native/scalar_native_callback_array_record_field_mutation.sn': '3a33bf7b2471ba94b4ed391aa7df7377040c6620d5d7c3847429aea4a663302d',
 'tests/rust-native/scalar_native_callback_array_record_pod_values.sn': '5d41a63d83628550d80ab57d1e3a89e09364e581bc9007d544026f373f62faf1',
 'tests/rust-native/scalar_native_callback_array_record_values.sn': '5506f9b96768d501509403db0dcfa8595999d3d5cc02a53302ecf741554c494b',
 'tests/rust-native/scalar_native_callback_array_string_alias.sn': '6b79db01013aa2ee8f2979eef8815ca8d4025f1cde224f910e1c67847777fb1c',
 'tests/rust-native/scalar_native_callback_array_string_captured_alias.sn': '84b73c10ccedfcabc419684bd30c37afde22b7bac4b539f106322d04fce21dcd',
 'tests/rust-native/scalar_native_callback_array_string_captured_private_snapshot.sn': '76ef66686b5964b7b230123e5afb4b2b4c75fafb7502b8739ce38c94199cc0bb',
 'tests/rust-native/scalar_native_callback_array_string_captured_repeat.sn': 'bdf74b76c6c83fd686e9067ed383b6537000d3e78a1c991ee99b7846be3439fb',
 'tests/rust-native/scalar_native_callback_array_string_cell_alias.sn': 'e5eca62c39564ede3546bc1cfab516cd27a805e4ce9506f7e51cd426771ad24e',
 'tests/rust-native/scalar_native_callback_array_string_cell_nested_mutation.sn': '8c2a6f29ec06effd54c35866f38f1cc83c50f6b0c923414f9efdd40ae5c6573e',
 'tests/rust-native/scalar_native_callback_array_string_cell_reentry.sn': 'c66c2b45a34c314ab6bdf72e9695801d94298f109f19768154bfc5b0f4804d96',
 'tests/rust-native/scalar_native_callback_array_string_cell_write.sn': 'd9b000b7ee11767a3ff5544bf5eec4720a499f33d389c4cb1f574f6147dfbe5c',
 'tests/rust-native/scalar_native_callback_array_string_field_alias.sn': 'c0309cc66bf30c53bd4d668de9726a839fc9e374e7e3a79768fcc112530bdec4',
 'tests/rust-native/scalar_native_callback_array_string_foreign.sn': '76dc460aecd2a95e55da483650f5faafd668079882b502b664c1aca08d82ef2d',
 'tests/rust-native/scalar_native_callback_array_string_hygiene.sn': '887fa44ab049373ad490f4b5de8a4786cb16d3b9627c8af063273bfd52074d7f',
 'tests/rust-native/scalar_native_callback_array_string_methods.sn': 'e7e2c49eb4d2005d4f0434a36df3953b1bb1e997d33637b699913b2500b0f195',
 'tests/rust-native/scalar_native_callback_array_string_native_alias.sn': 'a3ef4c5223b076e803119b8fc5b34ec9d3987ad6092939982b11478bdb0a774b',
 'tests/rust-native/scalar_native_callback_array_string_native_write.sn': '6563faab2c8c08b55899dc389e2bdac34b3016f803f19942c5c157ee3a280629',
 'tests/rust-native/scalar_native_callback_array_string_nil_elements.sn': 'e6676c7720223575d38d5e909110fb5c794c51d54436d1d32977d83c73689038',
 'tests/rust-native/scalar_native_callback_array_string_set.sn': '4ec367634e665efa72c8015aebd69e4608f226af97f4c2d36e3f71d687b42d9d',
 'tests/rust-native/scalar_native_callback_array_string_store_order.sn': '7a2970039d5b7d7da8856407d875427283b38e6279c3396444f7c54811bdc16a',
 'tests/rust-native/scalar_native_callback_array_string_temporary_alias.sn': '1ffb2f7609f52e8b82d9d2345b765b0a7648836533fd55a0c25e61b8d650e0f6',
 'tests/rust-native/scalar_native_callback_array_string_temporary_scope.sn': '428b9a8f67ee11a13f09eb676ddf3f48bba03795572d8c3fa90a32bfc3d9ffcc',
 'tests/rust-native/scalar_native_callback_array_string_temporary_statement.sn': '3ece0eca40d56efbea643bf474d71e807c96269378baec50e7f1059b8f3634c7',
 'tests/rust-native/scalar_native_callback_array_string_values.sn': '21079d8b25c4f067207d908b0b23cf7ee0c0c6b2a3b65b57a1326025ba6b63d2',
 'tests/rust-native/scalar_native_callback_char_bridge.sn': 'c66c3eabbbde5ad0937a8ad53614dd85f64ed90f91220592971e56bf3055529c',
 'tests/rust-native/scalar_native_callback_char_escaped_pointer.sn': '3a23dd069877dbe5ad782bf7adb871e4bd852a6da9c4541c2dbcf4ec8d99ef77',
 'tests/rust-native/scalar_native_callback_char_field_c_copy.sn': 'aab5eb4190a37f517625e385a57efa6ee495de8830e48d4fb925cf11ca35724a',
 'tests/rust-native/scalar_native_callback_char_retained_field.sn': '87a8bc88f7fadfd3336e249a9acc59ed056d14511c7e5cfd8e30ccee77836551',
 'tests/rust-native/scalar_native_callback_field_reentry.sn': '9138ef872085582f939891decb82e3b087fcd68fae9d32825b213180d9ffb0ac',
 'tests/rust-native/scalar_native_callback_foreign_repeat.sn': '36b86bd22f7e8617fff319bb746d724861e3b5d4142e95a062c660fdbf33cdec',
 'tests/rust-native/scalar_native_callback_identity.sn': '4cb385c2319b2182537f4792ce21f107ac82a64a68911fb8e8cf8a576df70294',
 'tests/rust-native/scalar_native_callback_managed_record_escape_snapshot.sn': '291e81605b839eac18fabb330f666172cd96fd529f706befcd3748f3b35c08b8',
 'tests/rust-native/scalar_native_callback_managed_record_snapshot_escape.sn': '5ce028848239c036daad29454bbe128546553eb05b8db848f3bfdc4066746e52',
 'tests/rust-native/scalar_native_callback_managed_record_snapshot_fields.sn': 'a1ec5be0a0c51dd717518629e03f81e829d83dc4bcc9a8e904cf19a1c8757ca1',
 'tests/rust-native/scalar_native_callback_managed_record_snapshot_forwarded.sn': 'ad846f5541d0b729419974574829ce6dda78f37de315b51df009a7eb602a3aeb',
 'tests/rust-native/scalar_native_callback_managed_record_snapshot_nested.sn': 'c2393785ea0ae39df3f0e205ed13244448966993ca0ee13fa6e81ae76d998d34',
 'tests/rust-native/scalar_native_callback_native_nil_result.sn': 'b5da981dbcfb4ea745ca26b67860d0c7423d9093479f023bdda8280b547f20e7',
 'tests/rust-native/scalar_native_callback_native_produced.sn': '0c80ba6df49894917546e916e716d5fa342ae7a079b707faa104c19ad9c28a0d',
 'tests/rust-native/scalar_native_callback_native_return.sn': 'f501bdef85f34e9519f973f3b06c4bca62038442c83a6405ca26a506a405772d',
 'tests/rust-native/scalar_native_callback_nil_roundtrip.sn': '6c2b0d669cf46adbc939a0cab9194e1d879f0cd5140301c27b5afba598d88d9e',
 'tests/rust-native/scalar_native_callback_optional_named.sn': '2b55fb1e5cdcdc8d560fce7f9832f7357404c8f4bf83dae77a39ca247c5f6c68',
 'tests/rust-native/scalar_native_callback_optional_recursive.sn': '8e31c93d0bbd5792eba3be25dd6f61e2c233020accc69424464138654d05bfcc',
 'tests/rust-native/scalar_native_callback_parallel_credits.sn': '28133d4f3e8fb371cedd098daea85683f4553a74ed78d1aa2cef938218d4b286',
 'tests/rust-native/scalar_native_callback_retained.sn': '9ed369ad4f434adcb2f59f9d0d44e8a608f4464d7ffc93f50d7e15cb4735e835',
 'tests/rust-native/scalar_native_callback_retained_identity.sn': '9a022ec32d6fd7623736bf8c438bc758a60986a11e39c2d2e7a994a5c4e9c971',
 'tests/rust-native/scalar_native_callback_scalar_write_reentry.sn': '0d24faa7fae288048de2948b996befab57b291c6945a56230898399ae4b0b62d',
 'tests/rust-native/scalar_native_callback_string_effect_order.sn': '11c4e26a2a6cb18e2f500846c663437e54e65b4a01e14421fc1e34b5551cd05a',
 'tests/rust-native/scalar_native_callback_string_invoke.sn': '95caeeaba1ea7a97596fde4e72931977d13f9e43f77997221f55e721498d84dc',
 'tests/rust-native/scalar_native_callback_string_native_mutate.sn': 'cf25c1a91cfab0e4f04cae04d9f9617564af5b6985cf9e15e7c3f2fd9fe84168',
 'tests/rust-native/scalar_native_callback_string_produced.sn': '47dbb0b6d3d230cf2c731884d0d3a8eae0f166517284da99d1876b365bb341b0',
 'tests/rust-native/scalar_native_callback_string_retained_input.sn': '5d59d9d1f0385c7b3ee84aada4e451d158eee91e0fdbb505441aba3f8bb3ed12',
 'tests/rust-native/scalar_native_callback_thread_foreign.sn': '7edf4471ce23a0a0843fcdf3e5418072c5e430afa72ec95a2eddf97c3507162b',
 'tests/rust-native/scalar_native_callback_thread_invoke.sn': '680469330ef9bc5285b6689c61fee78397872e3a62eb1bfd2c46b1274ba030c3',
 'tests/rust-native/scalar_native_callback_thread_retained_identity.sn': '3e596dca5ccad843b7733eb643a02542e3ec7f4c1bb171770dc8157fbeacde2e'}
HELPER_SHA256 = {'tests/rust-native/native_callback_array_char_foreign.sn.c': 'b2065338cb781d06ed50ceb70594e3bfa37b124caa12b3e2ba041248bbf7f387',
 'tests/rust-native/native_callback_array_string_foreign.sn.c': '80d3f8d8236b9836104bc0414fa634861f2c585cd3072743a33c27fda70ee581',
 'tests/rust-native/native_callback_array_string_native_alias.sn.c': '4b1e762dfcbe5aa910ee966e95d25e126addf60bfa2fb4b673ff243ec475f280',
 'tests/rust-native/native_callback_array_string_native_write.sn.c': '6eb01b2210f6acce5cc0336a7474344bad4b97779025df2ff4afd28ec4edd813',
 'tests/rust-native/native_callback_array_string_temporary_alias.sn.c': '1b1a302fa690bafca9829858c35bbc58a419f1f0d80678c9e58fcd7ff68a2a36',
 'tests/rust-native/native_callback_char_escaped_pointer.sn.c': '55f2228e1a40e67da141013385f15d6e2cd27c1460432943c75572e93fe35fed',
 'tests/rust-native/native_callback_char_field_write.sn.c': '334af4dc8ecb6bed33bfb0930c5e85edb0d72f59410d65389dbeef954145b774',
 'tests/rust-native/native_callback_foreign_repeat.sn.c': '4e0cc84b8dfee8386aca2be3b8c6fb253443d7f261d8cec4300aca502da18655',
 'tests/rust-native/native_callback_parallel_credits.sn.c': '4686f5d5d0c6ce5fc6c76b135dea901b5452751abb0cf1a485f4ffbda3bae8ee',
 'tests/rust-native/native_callback_retained.sn.c': '2753966513ce9ec1df7d36012bcd3819d27956ac9b7cfab1ef63d57f2127aabc',
 'tests/rust-native/native_callback_retained_identity.sn.c': 'eaccc51b1f1efd80083643cb5895e4ec2651fbc5c843e9558a4cbc001c1c7a33',
 'tests/rust-native/native_callback_string_effect_order.sn.c': '757450a5001533aa21343f7497a57edb294c9c858d33579cf97153a678b01eff',
 'tests/rust-native/native_callback_string_native_mutate.sn.c': '5d3ebd9ec64ead30529f481abf9f790d9c0a2aa77fb46b4c8197e99842dd58aa',
 'tests/rust-native/native_callback_string_retained_input.sn.c': 'e6f19f5f13c1ad89938e985b644030e8c894f7854ef8d2003e90cbe86ed55fd7'}


def verify(path):
    for helper, expected_hash in HELPER_SHA256.items():
        if hashlib.sha256(Path(helper).read_bytes()).hexdigest() != expected_hash:
            raise ValueError(f'native helper changed: {helper}')
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ('-O0', '-O1', '-O2')
                for mode in ('default', 'checked', 'unchecked')}
    if not report['passed'] or len(report['cases']) != len(required):
        raise ValueError('expected 189 successful borrowed scalar capture cases')
    seen = set()
    for case in report['cases']:
        source = case['source'].replace('\\', '/')
        identity = (source, case['optimization'], case['arithmetic_mode'])
        if not case['passed'] or identity not in required or identity in seen:
            raise ValueError(f'unexpected or duplicate case: {identity}')
        seen.add(identity)
        if (hashlib.sha256(Path(source).read_bytes()).hexdigest() != SOURCE_SHA256[source]
                or case['source_sha256'] != SOURCE_SHA256[source]):
            raise ValueError(f'source changed: {source}')
        expected = ORACLES[source]
        if Path(source).with_suffix('.expected').read_bytes() != expected:
            raise ValueError(f'stale fixture oracle: {source}')
        if os.name == 'nt':
            expected = expected.replace(b'\n', b'\r\n')
        if set(case['targets']) != {'c', 'rust'}:
            raise ValueError(f'missing target: {identity}')
        for target in ('c', 'rust'):
            result = case['targets'][target]
            if result['compile']['status'] != 0 or result['run']['status'] != 0:
                raise ValueError(f'unsuccessful execution: {identity} {target}')
            if result['run']['stdout_hex'] != expected.hex() or result['run']['stderr_hex']:
                raise ValueError(f'independent output mismatch: {identity} {target}')
    if seen != required:
        raise ValueError('incomplete optimization/arithmetic coverage')
    report['independent_oracle_cases'] = len(seen)
    report['native_source_sha256'] = HELPER_SHA256
    report['oracle_scope'] = 'Real C callback headers, identity, nil, scalar/string/record/array transport, reentry, retained pointers, snapshots and joined thread ownership; broader qualifier and concurrent access guarantees remain separate.'
    path.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: 612 independent native callback C/Rust oracles')


if __name__ == '__main__':
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        raise SystemExit(1)
