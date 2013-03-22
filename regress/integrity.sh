#	$OpenBSD: integrity.sh,v 1.7 2013/02/20 08:27:50 djm Exp $
#	Placed in the Public Domain.

tid="integrity"

# start at byte 2900 (i.e. after kex) and corrupt at different offsets
# XXX the test hangs if we modify the low bytes of the packet length
# XXX and ssh tries to read...
tries=10
startoffset=2900
macs="hmac-sha1 hmac-md5 umac-64@openssh.com umac-128@openssh.com
	hmac-sha1-96 hmac-md5-96 
	hmac-sha1-etm@openssh.com hmac-md5-etm@openssh.com
	umac-64-etm@openssh.com umac-128-etm@openssh.com
	hmac-sha1-96-etm@openssh.com hmac-md5-96-etm@openssh.com"
config_defined HAVE_EVP_SHA256 &&
	macs="$macs hmac-sha2-256 hmac-sha2-512
		hmac-sha2-256-etm@openssh.com hmac-sha2-512-etm@openssh.com"
# The following are not MACs, but ciphers with integrated integrity. They are
# handled specially below.
config_defined OPENSSL_HAVE_EVPGCM && \
	macs="$macs aes128-gcm@openssh.com aes256-gcm@openssh.com"

# sshd-command for proxy (see test-exec.sh)
cmd="$SUDO sh ${SRC}/sshd-log-wrapper.sh ${SSHD} ${TEST_SSH_LOGFILE} -i -f $OBJ/sshd_proxy"

jot() {
	awk "BEGIN { for (i = $2; i < $2 + $1; i++) { printf \"%d\n\", i } exit }"
}

for m in $macs; do
	trace "test $tid: mac $m"
	elen=0
	epad=0
	emac=0
	ecnt=0
	skip=0
	for off in `jot $tries $startoffset`; do
		skip=`expr $skip - 1`
		if [ $skip -gt 0 ]; then
			# avoid modifying the high bytes of the length
			continue
		fi
		# modify output from sshd at offset $off
		pxy="proxycommand=$cmd | $OBJ/modpipe -wm xor:$off:1"
		case $m in
			aes*gcm*)	macopt="-c $m";;
			*)		macopt="-m $m";;
		esac
		output=`${SSH} $macopt -2F $OBJ/ssh_proxy -o "$pxy" \
		    999.999.999.999 'printf "%4096s" " "' 2>&1`
		if [ $? -eq 0 ]; then
			fail "ssh -m $m succeeds with bit-flip at $off"
		fi
		ecnt=`expr $ecnt + 1`
		output=`echo $output | tr -s '\r\n' '.'`
		verbose "test $tid: $m @$off $output"
		case "$output" in
		Bad?packet*)	elen=`expr $elen + 1`; skip=3;;
		Corrupted?MAC* | Decryption?integrity?check?failed*)
				emac=`expr $emac + 1`; skip=0;;
		padding*)	epad=`expr $epad + 1`; skip=0;;
		*)		fail "unexpected error mac $m at $off";;
		esac
	done
	verbose "test $tid: $ecnt errors: mac $emac padding $epad length $elen"
	if [ $emac -eq 0 ]; then
		fail "$m: no mac errors"
	fi
	expect=`expr $ecnt - $epad - $elen`
	if [ $emac -ne $expect ]; then
		fail "$m: expected $expect mac errors, got $emac"
	fi
done
