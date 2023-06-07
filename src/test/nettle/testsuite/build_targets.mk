TARGET   := tests

TESTS := aes-test
TESTS += arcfour-test
TESTS += arctwo-test
TESTS += base16-test
TESTS += base64-test
TESTS += bcrypt-test
# TESTS += bignum-test           <-- hogweed needed, deactivated
TESTS += blowfish-test
TESTS += buffer-test
TESTS += camellia-test
TESTS += cast128-test
TESTS += cbc-test
TESTS += ccm-test
TESTS += cfb-test
TESTS += chacha-poly1305-test
TESTS += chacha-test
TESTS += cmac-test
TESTS += cnd-memcpy-test
# TESTS += ctr-test              <-- additional lib nettle_des3, deactivated
TESTS += curve25519-dh-test
TESTS += curve448-dh-test
# TESTS += cxx-test              <-- C++, implicit rule not working, deactivated
# TESTS += des3-test             <-- additional lib nettle_des3, deactivated
TESTS += des-test
# TESTS += dlopen-test           <-- failed on host, deactivated
# TESTS += dsa-keygen-test       <-- build error, hogweed needed, deactivated
# TESTS += dsa-test              <-- build error, hogweed needed, deactivated
TESTS += eax-test
# TESTS += ecc-add-test          <-- build error, hogweed needed, deactivated
# TESTS += ecc-dup-test          <-- build error, hogweed needed, deactivated
# TESTS += ecc-modinv-test       <-- build error, hogweed needed, deactivated
# TESTS += ecc-mod-test          <-- build error, hogweed needed, deactivated
# TESTS += ecc-mul-a-test        <-- build error, hogweed needed, deactivated
# TESTS += ecc-mul-g-test        <-- build error, hogweed needed, deactivated
# TESTS += ecc-redc-test         <-- build error, hogweed needed, deactivated
# TESTS += ecc-sqrt-test         <-- build error, hogweed needed, deactivated
# TESTS += ecdh-test             <-- build error, hogweed needed, deactivated
# TESTS += ecdsa-keygen-test     <-- build error, hogweed needed, deactivated
# TESTS += ecdsa-sign-test       <-- build error, hogweed needed, deactivated
# TESTS += ecdsa-verify-test     <-- build error, hogweed needed, deactivated
TESTS += ed25519-test
TESTS += ed448-test
TESTS += eddsa-compress-test
# TESTS += eddsa-sign-test       <-- build error, hogweed needed, deactivated
# TESTS += eddsa-verify-test     <-- build error, hogweed needed, deactivated
TESTS += gcm-test
# TESTS += gostdsa-keygen-test   <-- build error, hogweed needed, deactivated
# TESTS += gostdsa-sign-test     <-- build error, hogweed needed, deactivated
# TESTS += gostdsa-verify-test   <-- build error, hogweed needed, deactivated
# TESTS += gostdsa-vko-test      <-- build error, hogweed needed, deactivated
TESTS += gosthash94-test
TESTS += hkdf-test
TESTS += hmac-test
TESTS += knuth-lfib-test
TESTS += md2-test
TESTS += md4-test
TESTS += md5-compat-test
TESTS += md5-test
TESTS += memeql-test
# TESTS += memxor-test           <-- temporary deactivated as run > 5s
TESTS += meta-aead-test
TESTS += meta-armor-test
TESTS += meta-cipher-test
TESTS += meta-hash-test
TESTS += meta-mac-test
# TESTS += nettle-pbkdf2-test    <-- fails on host, deactivated
TESTS += pbkdf2-test
# TESTS += pkcs1-conv-test       <-- skipped on host, deactivated
TESTS += pkcs1-sec-decrypt-test
TESTS += pkcs1-test
TESTS += poly1305-test
TESTS += pss-mgf1-test
TESTS += pss-test
TESTS += random-prime-test
TESTS += ripemd160-test
# TESTS += rsa2sexp-test         <-- build error, would need rsa2sexp.c, deactivated
# TESTS += rsa-compute-root-test <-- build error, hogweed needed, deactivated
# TESTS += rsa-encrypt-test      <-- build error, hogweed needed, deactivated
# TESTS += rsa-keygen-test       <-- build error, hogweed needed, deactivated
# TESTS += rsa-pss-sign-tr-test  <-- build error, hogweed needed, deactivated
# TESTS += rsa-sec-decrypt-test  <-- build error, hogweed needed, deactivated
# TESTS += rsa-sign-tr-test      <-- build error, hogweed needed, deactivated
# TESTS += rsa-test              <-- build error, hogweed needed, deactivated
TESTS += salsa20-test
TESTS += serpent-test
# TESTS += sexp2rsa-test         <-- build/link error, ???, deactivated
# TESTS += sexp-conv-test        <-- fails on host, deactivated
# TESTS += sexp-format-test      <-- build/link error, ???, deactivated
# TESTS += sexp-test             <-- build/link error, ???, deactivated
# TESTS += sha1-huge-test        <-- temporary deactivated as run > 5s
TESTS += sha1-test
TESTS += sha224-test
TESTS += sha256-test
TESTS += sha3-224-test
TESTS += sha3-256-test
TESTS += sha3-384-test
TESTS += sha3-512-test
TESTS += sha384-test
TESTS += sha3-permute-test
TESTS += sha512-224-test
TESTS += sha512-256-test
TESTS += sha512-test
TESTS += shake256-test
TESTS += siv-test
TESTS += streebog-test
# TESTS += symbols-test          <-- no c code, deactivated
TESTS += twofish-test
TESTS += umac-test
TESTS += version-test
# TESTS += x86-ibt-test          <-- skipped on host, deactivated
TESTS += xts-test
# TESTS += yarrow-test           <-- fails on host, deactivated

tests: $(TESTS)

mkfile_path := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

%-test: nettle_test.template
	echo $@
	mkdir $(mkfile_path)$@
	sed 's/@@test_exe@@/$@/g' $(mkfile_path)nettle_test.template > $(mkfile_path)$@/target.mk