/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

var assert = require('assert');
var bser = require('../');
var Int64 = require('node-int64');

// This is a hard-coded template representation from the C test suite
var template =  "\x00\x01\x03\x28" +
                "\x0b\x00\x03\x02\x02\x03\x04\x6e\x61\x6d\x65\x02" +
                "\x03\x03\x61\x67\x65\x03\x03\x02\x03\x04\x66\x72" +
                "\x65\x64\x03\x14\x02\x03\x04\x70\x65\x74\x65\x03" +
                "\x1e\x0c\x03\x19" ;

var val = bser.loadFromBuffer(template);
assert.deepStrictEqual(val, [
  {"name": "fred", "age": 20},
  {"name": "pete", "age": 30},
  {"age": 25}
]);

function roundtrip(val) {
  var encoded = bser.dumpToBuffer(val);
  var decoded = bser.loadFromBuffer(encoded);
  assert.deepStrictEqual(decoded, val);
}

var values_to_test = [
  1,
  "hello",
  1.5,
  false,
  true,
  new Int64('0x0123456789abcdef'),
  127,
  128,
  129,
  32767,
  32768,
  32769,
  65534,
  65536,
  65537,
  2147483647,
  2147483648,
  2147483649,
  null,
  [1, 2, 3],
  {foo: "bar"},
  {nested: {struct: "hello", list: [true, false, 1, "string"]}}
];

for (var i = 0; i < values_to_test.length; ++i) {
  roundtrip(values_to_test[i]);
}
roundtrip(values_to_test);

// Verify Accumulator edge cases
var acc = new bser.Accumulator(8);
acc.append("hello");
assert.equal(acc.readAvail(), 5);
assert.equal(acc.readOffset, 0);
assert.equal(acc.readString(3), "hel");
assert.equal(acc.readOffset, 3);
assert.equal(acc.readAvail(), 2);
assert.equal(acc.writeAvail(), 3);

// This should trigger a shunt and not make the buffer bigger
acc.reserve(5);
assert.equal(acc.readOffset, 0, 'shunted');
assert.equal(acc.readAvail(), 2, 'still have 2 available to read');
assert.equal(acc.writeAvail(), 6, '2 left to read out of 8 total space');
assert.equal(acc.peekString(2), 'lo', 'have the correct remainder');

// Don't include keys that have undefined values
var res = bser.dumpToBuffer({expression: undefined});
assert.deepStrictEqual(bser.loadFromBuffer(res), {});

// Dump numbers without fraction to integers
var buffer;
buffer = bser.dumpToBuffer(1);
assert.equal(buffer.toString('hex'), "000105020000000301");
buffer = bser.dumpToBuffer(1.0);
assert.equal(buffer.toString('hex'), "000105020000000301");

// Dump numbers with fraction to double
buffer = bser.dumpToBuffer(1.1);
assert.equal(buffer.toString('hex'), "00010509000000079a9999999999f13f");

