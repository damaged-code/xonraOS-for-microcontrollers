#!/usr/bin/env node
// Sign a firmware build so boards can install it over Wi-Fi (Settings > Firmware).
//
// Usage:
//   node tools/release-firmware.js <xonraOS.ino.bin> <private.key> <output folder>
//
// It writes two files into the output folder, which you publish to your app server under /firmware/:
//   xonraos.bin   the build with an RSA-SHA256 signature appended (the format the ESP8266 Updater expects)
//   version.txt   the FW_VERSION number read from the firmware source
//
// The board only installs builds signed by the private key that matches firmware/xonraOS/public.key.
// Keep private.key secret and out of git. To use your own keys:
//   openssl genrsa -out private.key 2048
//   openssl rsa -in private.key -pubout -out firmware/xonraOS/public.key
// then build with that public.key in the sketch folder, flash once over USB, and sign releases with private.key.
//
// Build the .bin with:  arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 --output-dir build firmware/xonraOS

const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

const [, , binPath, keyPath, outDir] = process.argv;
if (!binPath || !keyPath || !outDir) {
  console.error('Usage: node tools/release-firmware.js <xonraOS.ino.bin> <private.key> <output folder>');
  process.exit(1);
}

const src = fs.readFileSync(path.join(__dirname, '..', 'firmware', 'xonraOS', 'xonraOS.ino'), 'utf8');
const m = src.match(/const int FW_VERSION = (\d+);/);
if (!m) { console.error('Could not find FW_VERSION in xonraOS.ino'); process.exit(1); }

const bin = fs.readFileSync(binPath);
const key = fs.readFileSync(keyPath);
const sig = crypto.createSign('RSA-SHA256').update(bin).sign(key);  // same as: openssl dgst -sha256 -sign private.key
const len = Buffer.alloc(4);
len.writeUInt32LE(sig.length);

fs.mkdirSync(outDir, { recursive: true });
fs.writeFileSync(path.join(outDir, 'xonraos.bin'), Buffer.concat([bin, sig, len]));
fs.writeFileSync(path.join(outDir, 'version.txt'), m[1] + '\n');
console.log(`Signed firmware v${m[1]}: ${bin.length} bytes + ${sig.length}-byte signature -> ${path.join(outDir, 'xonraos.bin')}`);
