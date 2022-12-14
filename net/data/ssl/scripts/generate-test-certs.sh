#!/bin/sh

# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script generates a set of test (end-entity, intermediate, root)
# certificates that can be used to test fetching of an intermediate via AIA.

try() {
  "$@" || (e=$?; echo "$@" > /dev/stderr; exit $e)
}

try rm -rf out
try mkdir out

try /bin/sh -c "echo 01 > out/2048-sha256-root-serial"
touch out/2048-sha256-root-index.txt

# Generate the key
try openssl genrsa -out out/2048-sha256-root.key 2048

# Generate the root certificate
CA_COMMON_NAME="Test Root CA" \
  try openssl req \
    -new \
    -key out/2048-sha256-root.key \
    -out out/2048-sha256-root.req \
    -config ca.cnf

CA_COMMON_NAME="Test Root CA" \
  try openssl x509 \
    -req -days 3650 \
    -in out/2048-sha256-root.req \
    -signkey out/2048-sha256-root.key \
    -extfile ca.cnf \
    -extensions ca_cert \
    -text > out/2048-sha256-root.pem

# Generate the leaf certificate requests
try openssl req \
  -new \
  -keyout out/expired_cert.key \
  -out out/expired_cert.req \
  -config ee.cnf

try openssl req \
  -new \
  -keyout out/ok_cert.key \
  -out out/ok_cert.req \
  -config ee.cnf

SUBJECT_NAME=req_localhost_cn \
try openssl req \
  -new \
  -keyout out/localhost_cert.key \
  -out out/localhost_cert.req \
  -reqexts req_localhost_san \
  -config ee.cnf

# Generate the leaf certificates
CA_COMMON_NAME="Test Root CA" \
  try openssl ca \
    -batch \
    -extensions user_cert \
    -startdate 060101000000Z \
    -enddate 070101000000Z \
    -in out/expired_cert.req \
    -out out/expired_cert.pem \
    -config ca.cnf

CA_COMMON_NAME="Test Root CA" \
  try openssl ca \
    -batch \
    -extensions user_cert \
    -days 3650 \
    -in out/ok_cert.req \
    -out out/ok_cert.pem \
    -config ca.cnf

CA_COMMON_NAME="Test Root CA" \
  try openssl ca \
    -batch \
    -extensions name_constraint_bad \
    -subj "/CN=Leaf certificate/" \
    -days 3650 \
    -in out/ok_cert.req \
    -out out/name_constraint_bad.pem \
    -config ca.cnf

CA_COMMON_NAME="Test Root CA" \
  try openssl ca \
    -batch \
    -extensions name_constraint_good \
    -subj "/CN=Leaf Certificate/" \
    -days 3650 \
    -in out/ok_cert.req \
    -out out/name_constraint_good.pem \
    -config ca.cnf

CA_COMMON_NAME="Test Root CA" \
  try openssl ca \
    -batch \
    -extensions user_cert \
    -days 3650 \
    -in out/localhost_cert.req \
    -out out/localhost_cert.pem \
    -config ca.cnf

CA_COMMON_NAME="Test Root CA" \
  try openssl ca \
    -batch \
    -extensions user_cert \
    -subj "/CN=Leaf Certificate/" \
    -startdate 00010101000000Z \
    -enddate   00010101000000Z \
    -in out/ok_cert.req \
    -out out/bad_validity.pem \
    -config ca.cnf

try /bin/sh -c "cat out/ok_cert.key out/ok_cert.pem \
    > ../certificates/ok_cert.pem"
try /bin/sh -c "cat out/localhost_cert.key out/localhost_cert.pem \
    > ../certificates/localhost_cert.pem"
try /bin/sh -c "cat out/expired_cert.key out/expired_cert.pem \
    > ../certificates/expired_cert.pem"
try /bin/sh -c "cat out/2048-sha256-root.key out/2048-sha256-root.pem \
    > ../certificates/root_ca_cert.pem"
try /bin/sh -c "cat out/ok_cert.key out/name_constraint_bad.pem \
    > ../certificates/name_constraint_bad.pem"
try /bin/sh -c "cat out/ok_cert.key out/name_constraint_good.pem \
    > ../certificates/name_constraint_good.pem"
try /bin/sh -c "cat out/ok_cert.key out/bad_validity.pem \
    > ../certificates/bad_validity.pem"

# Now generate the one-off certs
## SHA-256 general test cert
try openssl req -x509 -days 3650 \
    -config ../scripts/ee.cnf -newkey rsa:2048 -text \
    -sha256 \
    -out ../certificates/sha256.pem

## Self-signed cert for SPDY/QUIC/HTTP2 pooling testing
try openssl req -x509 -days 3650 -extensions req_spdy_pooling \
    -config ../scripts/ee.cnf -newkey rsa:2048 -text \
    -out ../certificates/spdy_pooling.pem

## SubjectAltName parsing
try openssl req -x509 -days 3650 -extensions req_san_sanity \
    -config ../scripts/ee.cnf -newkey rsa:2048 -text \
    -out ../certificates/subjectAltName_sanity_check.pem

## Punycode handling
SUBJECT_NAME="req_punycode_dn" \
  try openssl req -x509 -days 3650 -extensions req_punycode \
    -config ../scripts/ee.cnf -newkey rsa:2048 -text \
    -out ../certificates/punycodetest.pem

## Reject intranet hostnames in "publicly" trusted certs
# 365 * 3 = 1095
SUBJECT_NAME="req_dn" \
  try openssl req -x509 -days 1095 \
    -config ../scripts/ee.cnf -newkey rsa:2048 -text \
    -out ../certificates/reject_intranet_hosts.pem

## Leaf certificate with a large key; Apple's certificate verifier rejects with
## a fatal error if the key is bigger than 4096 bits.
try openssl req -x509 -days 3650 \
    -config ../scripts/ee.cnf -newkey rsa:4104 -text \
    -sha256 \
    -out ../certificates/large_key.pem

## SHA1 certificate expiring in 2016.
try openssl req -config ../scripts/ee.cnf -sha1 \
  -newkey rsa:2048 -text -out out/sha1_2016.req
CA_COMMON_NAME="Test Root CA" \
  try openssl ca \
    -batch \
    -extensions user_cert \
    -startdate 081030000000Z \
    -enddate   161230000000Z \
    -in out/sha1_2016.req \
    -out ../certificates/sha1_2016.pem \
    -config ca.cnf

## SHA1 certificate issued the last second before the SHA-1 deprecation date.
try openssl req -config ../scripts/ee.cnf -sha1 \
  -newkey rsa:2048 -text -out out/sha1_dec_2015.req
CA_COMMON_NAME="Test Root CA" \
  try openssl ca \
    -batch \
    -extensions user_cert \
    -startdate 151231235959Z \
    -enddate   161230000000Z \
    -in out/sha1_dec_2015.req \
    -out ../certificates/sha1_dec_2015.pem \
    -config ca.cnf

## SHA1 certificate issued on the SHA-1 deprecation date.
try openssl req -config ../scripts/ee.cnf -sha1 \
  -newkey rsa:2048 -text -out out/sha1_jan_2016.req
CA_COMMON_NAME="Test Root CA" \
  try openssl ca \
    -batch \
    -extensions user_cert \
    -startdate 160101000000Z \
    -enddate   161230000000Z \
    -in out/sha1_jan_2016.req \
    -out ../certificates/sha1_jan_2016.pem \
    -config ca.cnf

## Validity too long unit test support.
try openssl req -config ../scripts/ee.cnf \
  -newkey rsa:2048 -text -out ../certificates/10_year_validity.req
CA_COMMON_NAME="Test Root CA" \
  try openssl ca \
    -batch \
    -extensions user_cert \
    -startdate 081030000000Z \
    -enddate   181029000000Z \
    -in ../certificates/10_year_validity.req \
    -out ../certificates/10_year_validity.pem \
    -config ca.cnf
# 365 * 11 = 4015
try openssl req -config ../scripts/ee.cnf \
  -newkey rsa:2048 -text -out ../certificates/11_year_validity.req
CA_COMMON_NAME="Test Root CA" \
  try openssl ca \
    -batch \
    -extensions user_cert \
    -startdate 141030000000Z \
    -days 4015 \
    -in ../certificates/11_year_validity.req \
    -out ../certificates/11_year_validity.pem \
    -config ca.cnf
try openssl req -config ../scripts/ee.cnf \
  -newkey rsa:2048 -text -out ../certificates/39_months_after_2015_04.req
CA_COMMON_NAME="Test Root CA" \
  try openssl ca \
    -batch \
    -extensions user_cert \
    -startdate 150402000000Z \
    -enddate   180702000000Z \
    -in ../certificates/39_months_after_2015_04.req \
    -out ../certificates/39_months_after_2015_04.pem \
    -config ca.cnf
try openssl req -config ../scripts/ee.cnf \
  -newkey rsa:2048 -text -out ../certificates/40_months_after_2015_04.req
CA_COMMON_NAME="Test Root CA" \
  try openssl ca \
    -batch \
    -extensions user_cert \
    -startdate 150402000000Z \
    -enddate   180801000000Z \
    -in ../certificates/40_months_after_2015_04.req \
    -out ../certificates/40_months_after_2015_04.pem \
    -config ca.cnf
try openssl req -config ../scripts/ee.cnf \
  -newkey rsa:2048 -text -out ../certificates/60_months_after_2012_07.req
CA_COMMON_NAME="Test Root CA" \
  try openssl ca \
    -batch \
    -extensions user_cert \
    -startdate 141030000000Z \
    -enddate   190930000000Z \
    -in ../certificates/60_months_after_2012_07.req \
    -out ../certificates/60_months_after_2012_07.pem \
    -config ca.cnf
try openssl req -config ../scripts/ee.cnf \
  -newkey rsa:2048 -text -out ../certificates/61_months_after_2012_07.req
# 30 * 61 = 1830
CA_COMMON_NAME="Test Root CA" \
  try openssl ca \
    -batch \
    -extensions user_cert \
    -startdate 141030000000Z \
    -days 1830 \
    -in ../certificates/61_months_after_2012_07.req \
    -out ../certificates/61_months_after_2012_07.pem \
    -config ca.cnf
# start date after expiry date
try openssl req -config ../scripts/ee.cnf \
  -newkey rsa:2048 -text -out ../certificates/start_after_expiry.req
CA_COMMON_NAME="Test Root CA" \
  try openssl ca \
    -batch \
    -extensions user_cert \
    -startdate 180901000000Z \
    -enddate   150402000000Z \
    -in ../certificates/start_after_expiry.req \
    -out ../certificates/start_after_expiry.pem \
    -config ca.cnf
try openssl req -config ../scripts/ee.cnf \
  -newkey rsa:2048 -text -out ../certificates/start_after_expiry.req
# Issued pre-BRs, lifetime < 120 months, expires before 2019-07-01
try openssl req -config ../scripts/ee.cnf \
  -newkey rsa:2048 -text -out ../certificates/pre_br_validity_ok.req
CA_COMMON_NAME="Test Root CA" \
  try openssl ca \
    -batch \
    -extensions user_cert \
    -startdate 080101000000Z \
    -enddate   150101000000Z \
    -in ../certificates/pre_br_validity_ok.req \
    -out ../certificates/pre_br_validity_ok.pem \
    -config ca.cnf
try openssl req -config ../scripts/ee.cnf \
  -newkey rsa:2048 -text -out ../certificates/pre_br_validity_ok.req
# Issued pre-BRs, lifetime > 120 months, expires before 2019-07-01
try openssl req -config ../scripts/ee.cnf \
  -newkey rsa:2048 -text -out ../certificates/pre_br_validity_bad_121.req
CA_COMMON_NAME="Test Root CA" \
  try openssl ca \
    -batch \
    -extensions user_cert \
    -startdate 080101000000Z \
    -enddate   180501000000Z \
    -in ../certificates/pre_br_validity_bad_121.req \
    -out ../certificates/pre_br_validity_bad_121.pem \
    -config ca.cnf
try openssl req -config ../scripts/ee.cnf \
  -newkey rsa:2048 -text -out ../certificates/pre_br_validity_bad_121.req
# Issued pre-BRs, lifetime < 120 months, expires after 2019-07-01
try openssl req -config ../scripts/ee.cnf \
  -newkey rsa:2048 -text -out ../certificates/pre_br_validity_bad_2020.req
CA_COMMON_NAME="Test Root CA" \
  try openssl ca \
    -batch \
    -extensions user_cert \
    -startdate 120501000000Z \
    -enddate   190703000000Z \
    -in ../certificates/pre_br_validity_bad_2020.req \
    -out ../certificates/pre_br_validity_bad_2020.pem \
    -config ca.cnf
try openssl req -config ../scripts/ee.cnf \
  -newkey rsa:2048 -text -out ../certificates/pre_br_validity_bad_2020.req

# Regenerate CRLSets
## Block a leaf cert directly by SPKI
try python crlsetutil.py -o ../certificates/crlset_by_leaf_spki.raw \
<<CRLBYLEAFSPKI
{
  "BlockedBySPKI": ["../certificates/ok_cert.pem"]
}
CRLBYLEAFSPKI

## Block a leaf cert by issuer-hash-and-serial (ok_cert.pem == serial 2, by
## virtue of the serial file and ordering above.
try python crlsetutil.py -o ../certificates/crlset_by_root_serial.raw \
<<CRLBYROOTSERIAL
{
  "BlockedByHash": {
    "../certificates/root_ca_cert.pem": [2]
  }
}
CRLBYROOTSERIAL

## Block a leaf cert by issuer-hash-and-serial. However, this will be issued
## from an intermediate CA issued underneath a root.
try python crlsetutil.py -o ../certificates/crlset_by_intermediate_serial.raw \
<<CRLSETBYINTERMEDIATESERIAL
{
  "BlockedByHash": {
    "../certificates/quic_intermediate.crt": [3]
  }
}
CRLSETBYINTERMEDIATESERIAL
