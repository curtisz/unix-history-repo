/*
 * EAP peer method: EAP-GPSK (draft-ietf-emu-eap-gpsk-08.txt)
 * Copyright (c) 2006-2007, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#include "includes.h"

#include "common.h"
#include "eap_i.h"
#include "config_ssid.h"
#include "eap_gpsk_common.h"

struct eap_gpsk_data {
	enum { GPSK_1, GPSK_3, SUCCESS, FAILURE } state;
	u8 rand_server[EAP_GPSK_RAND_LEN];
	u8 rand_peer[EAP_GPSK_RAND_LEN];
	u8 msk[EAP_MSK_LEN];
	u8 emsk[EAP_EMSK_LEN];
	u8 sk[EAP_GPSK_MAX_SK_LEN];
	size_t sk_len;
	u8 pk[EAP_GPSK_MAX_PK_LEN];
	size_t pk_len;
	u8 session_id;
	int session_id_set;
	u8 *id_peer;
	size_t id_peer_len;
	u8 *id_server;
	size_t id_server_len;
	int vendor; /* CSuite/Specifier */
	int specifier; /* CSuite/Specifier */
	u8 *psk;
	size_t psk_len;
};


static u8 * eap_gpsk_send_gpsk_2(struct eap_gpsk_data *data, u8 identifier,
				 const u8 *csuite_list, size_t csuite_list_len,
				 size_t *respDataLen);
static u8 * eap_gpsk_send_gpsk_4(struct eap_gpsk_data *data, u8 identifier,
				 size_t *respDataLen);


#ifndef CONFIG_NO_STDOUT_DEBUG
static const char * eap_gpsk_state_txt(int state)
{
	switch (state) {
	case GPSK_1:
		return "GPSK-1";
	case GPSK_3:
		return "GPSK-3";
	case SUCCESS:
		return "SUCCESS";
	case FAILURE:
		return "FAILURE";
	default:
		return "?";
	}
}
#endif /* CONFIG_NO_STDOUT_DEBUG */


static void eap_gpsk_state(struct eap_gpsk_data *data, int state)
{
	wpa_printf(MSG_DEBUG, "EAP-GPSK: %s -> %s",
		   eap_gpsk_state_txt(data->state),
		   eap_gpsk_state_txt(state));
	data->state = state;
}


static void eap_gpsk_deinit(struct eap_sm *sm, void *priv);


static void * eap_gpsk_init(struct eap_sm *sm)
{
	struct wpa_ssid *config = eap_get_config(sm);
	struct eap_gpsk_data *data;

	if (config == NULL) {
		wpa_printf(MSG_INFO, "EAP-GPSK: No configuration found");
		return NULL;
	}

	if (config->eappsk == NULL) {
		wpa_printf(MSG_INFO, "EAP-GPSK: No key (eappsk) configured");
		return NULL;
	}

	data = os_zalloc(sizeof(*data));
	if (data == NULL)
		return NULL;
	data->state = GPSK_1;

	if (config->nai) {
		data->id_peer = os_malloc(config->nai_len);
		if (data->id_peer == NULL) {
			eap_gpsk_deinit(sm, data);
			return NULL;
		}
		os_memcpy(data->id_peer, config->nai, config->nai_len);
		data->id_peer_len = config->nai_len;
	}

	data->psk = os_malloc(config->eappsk_len);
	if (data->psk == NULL) {
		eap_gpsk_deinit(sm, data);
		return NULL;
	}
	os_memcpy(data->psk, config->eappsk, config->eappsk_len);
	data->psk_len = config->eappsk_len;

	return data;
}


static void eap_gpsk_deinit(struct eap_sm *sm, void *priv)
{
	struct eap_gpsk_data *data = priv;
	os_free(data->id_server);
	os_free(data->id_peer);
	os_free(data->psk);
	os_free(data);
}


const u8 * eap_gpsk_process_id_server(struct eap_gpsk_data *data,
				      const u8 *pos, const u8 *end)
{
	u16 alen;

	if (end - pos < 2) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: Too short GPSK-1 packet");
		return NULL;
	}
	alen = WPA_GET_BE16(pos);
	pos += 2;
	if (end - pos < alen) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: ID_Server overflow");
		return NULL;
	}
	os_free(data->id_server);
	data->id_server = os_malloc(alen);
	if (data->id_server == NULL) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: No memory for ID_Server");
		return NULL;
	}
	os_memcpy(data->id_server, pos, alen);
	data->id_server_len = alen;
	wpa_hexdump_ascii(MSG_DEBUG, "EAP-GPSK: ID_Server",
			  data->id_server, data->id_server_len);
	pos += alen;

	return pos;
}


const u8 * eap_gpsk_process_rand_server(struct eap_gpsk_data *data,
					const u8 *pos, const u8 *end)
{
	if (pos == NULL)
		return NULL;

	if (end - pos < EAP_GPSK_RAND_LEN) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: RAND_Server overflow");
		return NULL;
	}
	os_memcpy(data->rand_server, pos, EAP_GPSK_RAND_LEN);
	wpa_hexdump(MSG_DEBUG, "EAP-GPSK: RAND_Server",
		    data->rand_server, EAP_GPSK_RAND_LEN);
	pos += EAP_GPSK_RAND_LEN;

	return pos;
}


static int eap_gpsk_select_csuite(struct eap_sm *sm,
				  struct eap_gpsk_data *data,
				  const u8 *csuite_list,
				  size_t csuite_list_len)
{
	struct eap_gpsk_csuite *csuite;
	int i, count;

	count = csuite_list_len / sizeof(struct eap_gpsk_csuite);
	data->vendor = EAP_GPSK_VENDOR_IETF;
	data->specifier = EAP_GPSK_CIPHER_RESERVED;
	csuite = (struct eap_gpsk_csuite *) csuite_list;
	for (i = 0; i < count; i++) {
		int vendor, specifier;
		vendor = WPA_GET_BE32(csuite->vendor);
		specifier = WPA_GET_BE16(csuite->specifier);
		wpa_printf(MSG_DEBUG, "EAP-GPSK: CSuite[%d]: %d:%d",
			   i, vendor, specifier);
		if (data->vendor == EAP_GPSK_VENDOR_IETF &&
		    data->specifier == EAP_GPSK_CIPHER_RESERVED &&
		    eap_gpsk_supported_ciphersuite(vendor, specifier)) {
			data->vendor = vendor;
			data->specifier = specifier;
		}
		csuite++;
	}
	if (data->vendor == EAP_GPSK_VENDOR_IETF &&
	    data->specifier == EAP_GPSK_CIPHER_RESERVED) {
		wpa_msg(sm->msg_ctx, MSG_INFO, "EAP-GPSK: No supported "
			"ciphersuite found");
		return -1;
	}
	wpa_printf(MSG_DEBUG, "EAP-GPSK: Selected ciphersuite %d:%d",
		   data->vendor, data->specifier);

	return 0;
}


const u8 * eap_gpsk_process_csuite_list(struct eap_sm *sm,
					struct eap_gpsk_data *data,
					const u8 **list, size_t *list_len,
					const u8 *pos, const u8 *end)
{
	if (pos == NULL)
		return NULL;

	if (end - pos < 2) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: Too short GPSK-1 packet");
		return NULL;
	}
	*list_len = WPA_GET_BE16(pos);
	pos += 2;
	if (end - pos < (int) *list_len) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: CSuite_List overflow");
		return NULL;
	}
	if (*list_len == 0 || (*list_len % sizeof(struct eap_gpsk_csuite))) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: Invalid CSuite_List len %lu",
			   (unsigned long) *list_len);
		return NULL;
	}
	*list = pos;
	pos += *list_len;

	if (eap_gpsk_select_csuite(sm, data, *list, *list_len) < 0)
		return NULL;

	return pos;
}


static u8 * eap_gpsk_process_gpsk_1(struct eap_sm *sm,
				    struct eap_gpsk_data *data,
				    struct eap_method_ret *ret,
				    const u8 *reqData, size_t reqDataLen,
				    const u8 *payload, size_t payload_len,
				    size_t *respDataLen)
{
	size_t csuite_list_len;
	const u8 *csuite_list, *pos, *end;
	const struct eap_hdr *req;
	u8 *resp;

	if (data->state != GPSK_1) {
		ret->ignore = TRUE;
		return NULL;
	}

	wpa_printf(MSG_DEBUG, "EAP-GPSK: Received Request/GPSK-1");

	end = payload + payload_len;

	pos = eap_gpsk_process_id_server(data, payload, end);
	pos = eap_gpsk_process_rand_server(data, pos, end);
	pos = eap_gpsk_process_csuite_list(sm, data, &csuite_list,
					   &csuite_list_len, pos, end);
	if (pos == NULL) {
		eap_gpsk_state(data, FAILURE);
		return NULL;
	}

	req = (const struct eap_hdr *) reqData;
	resp = eap_gpsk_send_gpsk_2(data, req->identifier,
				    csuite_list, csuite_list_len,
				    respDataLen);
	if (resp == NULL)
		return NULL;

	eap_gpsk_state(data, GPSK_3);

	return (u8 *) resp;
}


static u8 * eap_gpsk_send_gpsk_2(struct eap_gpsk_data *data, u8 identifier,
				 const u8 *csuite_list, size_t csuite_list_len,
				 size_t *respDataLen)
{
	struct eap_hdr *resp;
	size_t len, miclen;
	u8 *rpos, *start;
	struct eap_gpsk_csuite *csuite;

	wpa_printf(MSG_DEBUG, "EAP-GPSK: Sending Response/GPSK-2");

	miclen = eap_gpsk_mic_len(data->vendor, data->specifier);
	len = 1 + 2 + data->id_peer_len + 2 + data->id_server_len +
		2 * EAP_GPSK_RAND_LEN + 2 + csuite_list_len +
		sizeof(struct eap_gpsk_csuite) + 2 + miclen;

	resp = eap_msg_alloc(EAP_VENDOR_IETF, EAP_TYPE_GPSK, respDataLen, len,
			     EAP_CODE_RESPONSE, identifier, &rpos);
	if (resp == NULL)
		return NULL;

	*rpos++ = EAP_GPSK_OPCODE_GPSK_2;
	start = rpos;

	wpa_hexdump_ascii(MSG_DEBUG, "EAP-GPSK: ID_Peer",
			  data->id_peer, data->id_peer_len);
	WPA_PUT_BE16(rpos, data->id_peer_len);
	rpos += 2;
	if (data->id_peer)
		os_memcpy(rpos, data->id_peer, data->id_peer_len);
	rpos += data->id_peer_len;

	WPA_PUT_BE16(rpos, data->id_server_len);
	rpos += 2;
	if (data->id_server)
		os_memcpy(rpos, data->id_server, data->id_server_len);
	rpos += data->id_server_len;

	if (os_get_random(data->rand_peer, EAP_GPSK_RAND_LEN)) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: Failed to get random data "
			   "for RAND_Peer");
		eap_gpsk_state(data, FAILURE);
		os_free(resp);
		return NULL;
	}
	wpa_hexdump(MSG_DEBUG, "EAP-GPSK: RAND_Peer",
		    data->rand_peer, EAP_GPSK_RAND_LEN);
	os_memcpy(rpos, data->rand_peer, EAP_GPSK_RAND_LEN);
	rpos += EAP_GPSK_RAND_LEN;

	os_memcpy(rpos, data->rand_server, EAP_GPSK_RAND_LEN);
	rpos += EAP_GPSK_RAND_LEN;

	WPA_PUT_BE16(rpos, csuite_list_len);
	rpos += 2;
	os_memcpy(rpos, csuite_list, csuite_list_len);
	rpos += csuite_list_len;

	csuite = (struct eap_gpsk_csuite *) rpos;
	WPA_PUT_BE32(csuite->vendor, data->vendor);
	WPA_PUT_BE16(csuite->specifier, data->specifier);
	rpos = (u8 *) (csuite + 1);

	if (eap_gpsk_derive_keys(data->psk, data->psk_len,
				 data->vendor, data->specifier,
				 data->rand_peer, data->rand_server,
				 data->id_peer, data->id_peer_len,
				 data->id_server, data->id_server_len,
				 data->msk, data->emsk,
				 data->sk, &data->sk_len,
				 data->pk, &data->pk_len) < 0) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: Failed to derive keys");
		eap_gpsk_state(data, FAILURE);
		os_free(resp);
		return NULL;
	}

	/* No PD_Payload_1 */
	WPA_PUT_BE16(rpos, 0);
	rpos += 2;

	if (eap_gpsk_compute_mic(data->sk, data->sk_len, data->vendor,
				 data->specifier, start, rpos - start, rpos) <
	    0) {
		eap_gpsk_state(data, FAILURE);
		os_free(resp);
		return NULL;
	}

	return (u8 *) resp;
}


const u8 * eap_gpsk_validate_rand(struct eap_gpsk_data *data, const u8 *pos,
				  const u8 *end)
{
	if (end - pos < EAP_GPSK_RAND_LEN) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: Message too short for "
			   "RAND_Peer");
		return NULL;
	}
	if (os_memcmp(pos, data->rand_peer, EAP_GPSK_RAND_LEN) != 0) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: RAND_Peer in GPSK-2 and "
			   "GPSK-3 did not match");
		wpa_hexdump(MSG_DEBUG, "EAP-GPSK: RAND_Peer in GPSK-2",
			    data->rand_peer, EAP_GPSK_RAND_LEN);
		wpa_hexdump(MSG_DEBUG, "EAP-GPSK: RAND_Peer in GPSK-3",
			    pos, EAP_GPSK_RAND_LEN);
		return NULL;
	}
	pos += EAP_GPSK_RAND_LEN;

	if (end - pos < EAP_GPSK_RAND_LEN) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: Message too short for "
			   "RAND_Server");
		return NULL;
	}
	if (os_memcmp(pos, data->rand_server, EAP_GPSK_RAND_LEN) != 0) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: RAND_Server in GPSK-1 and "
			   "GPSK-3 did not match");
		wpa_hexdump(MSG_DEBUG, "EAP-GPSK: RAND_Server in GPSK-1",
			    data->rand_server, EAP_GPSK_RAND_LEN);
		wpa_hexdump(MSG_DEBUG, "EAP-GPSK: RAND_Server in GPSK-3",
			    pos, EAP_GPSK_RAND_LEN);
		return NULL;
	}
	pos += EAP_GPSK_RAND_LEN;

	return pos;
}


const u8 * eap_gpsk_validate_id_server(struct eap_gpsk_data *data,
				       const u8 *pos, const u8 *end)
{
	size_t len;

	if (pos == NULL)
		return NULL;

	if (end - pos < (int) 2) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: Message too short for "
			   "length(ID_Server)");
		return NULL;
	}

	len = WPA_GET_BE16(pos);
	pos += 2;

	if (end - pos < (int) len) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: Message too short for "
			   "ID_Server");
		return NULL;
	}

	if (len != data->id_server_len ||
	    os_memcmp(pos, data->id_server, len) != 0) {
		wpa_printf(MSG_INFO, "EAP-GPSK: ID_Server did not match with "
			   "the one used in GPSK-1");
		wpa_hexdump_ascii(MSG_DEBUG, "EAP-GPSK: ID_Server in GPSK-1",
				  data->id_server, data->id_server_len);
		wpa_hexdump_ascii(MSG_DEBUG, "EAP-GPSK: ID_Server in GPSK-3",
				  pos, len);
		return NULL;
	}

	pos += len;

	return pos;
}


const u8 * eap_gpsk_validate_csuite(struct eap_gpsk_data *data, const u8 *pos,
				    const u8 *end)
{
	int vendor, specifier;
	const struct eap_gpsk_csuite *csuite;

	if (pos == NULL)
		return NULL;

	if (end - pos < (int) sizeof(*csuite)) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: Message too short for "
			   "CSuite_Sel");
		return NULL;
	}
	csuite = (const struct eap_gpsk_csuite *) pos;
	vendor = WPA_GET_BE32(csuite->vendor);
	specifier = WPA_GET_BE16(csuite->specifier);
	pos += sizeof(*csuite);
	if (vendor != data->vendor || specifier != data->specifier) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: CSuite_Sel (%d:%d) does not "
			   "match with the one sent in GPSK-2 (%d:%d)",
			   vendor, specifier, data->vendor, data->specifier);
		return NULL;
	}

	return pos;
}


const u8 * eap_gpsk_validate_pd_payload_2(struct eap_gpsk_data *data,
					  const u8 *pos, const u8 *end)
{
	u16 alen;

	if (pos == NULL)
		return NULL;

	if (end - pos < 2) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: Message too short for "
			   "PD_Payload_2 length");
		return NULL;
	}
	alen = WPA_GET_BE16(pos);
	pos += 2;
	if (end - pos < alen) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: Message too short for "
			   "%d-octet PD_Payload_2", alen);
		return NULL;
	}
	wpa_hexdump(MSG_DEBUG, "EAP-GPSK: PD_Payload_2", pos, alen);
	pos += alen;

	return pos;
}


const u8 * eap_gpsk_validate_gpsk_3_mic(struct eap_gpsk_data *data,
					const u8 *payload,
					const u8 *pos, const u8 *end)
{
	size_t miclen;
	u8 mic[EAP_GPSK_MAX_MIC_LEN];

	if (pos == NULL)
		return NULL;

	miclen = eap_gpsk_mic_len(data->vendor, data->specifier);
	if (end - pos < (int) miclen) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: Message too short for MIC "
			   "(left=%lu miclen=%lu)",
			   (unsigned long) (end - pos),
			   (unsigned long) miclen);
		return NULL;
	}
	if (eap_gpsk_compute_mic(data->sk, data->sk_len, data->vendor,
				 data->specifier, payload, pos - payload, mic)
	    < 0) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: Failed to compute MIC");
		return NULL;
	}
	if (os_memcmp(mic, pos, miclen) != 0) {
		wpa_printf(MSG_INFO, "EAP-GPSK: Incorrect MIC in GPSK-3");
		wpa_hexdump(MSG_DEBUG, "EAP-GPSK: Received MIC", pos, miclen);
		wpa_hexdump(MSG_DEBUG, "EAP-GPSK: Computed MIC", mic, miclen);
		return NULL;
	}
	pos += miclen;

	return pos;
}


static u8 * eap_gpsk_process_gpsk_3(struct eap_sm *sm,
				    struct eap_gpsk_data *data,
				    struct eap_method_ret *ret,
				    const u8 *reqData, size_t reqDataLen,
				    const u8 *payload, size_t payload_len,
				    size_t *respDataLen)
{
	u8 *resp;
	const struct eap_hdr *req;
	const u8 *pos, *end;

	if (data->state != GPSK_3) {
		ret->ignore = TRUE;
		return NULL;
	}

	wpa_printf(MSG_DEBUG, "EAP-GPSK: Received Request/GPSK-3");

	end = payload + payload_len;

	pos = eap_gpsk_validate_rand(data, payload, end);
	pos = eap_gpsk_validate_id_server(data, pos, end);
	pos = eap_gpsk_validate_csuite(data, pos, end);
	pos = eap_gpsk_validate_pd_payload_2(data, pos, end);
	pos = eap_gpsk_validate_gpsk_3_mic(data, payload, pos, end);

	if (pos == NULL) {
		eap_gpsk_state(data, FAILURE);
		return NULL;
	}
	if (pos != end) {
		wpa_printf(MSG_DEBUG, "EAP-GPSK: Ignored %lu bytes of extra "
			   "data in the end of GPSK-2",
			   (unsigned long) (end - pos));
	}

	req = (const struct eap_hdr *) reqData;
	resp = eap_gpsk_send_gpsk_4(data, req->identifier, respDataLen);
	if (resp == NULL)
		return NULL;

	eap_gpsk_state(data, SUCCESS);
	ret->methodState = METHOD_DONE;
	ret->decision = DECISION_UNCOND_SUCC;

	return (u8 *) resp;
}


static u8 * eap_gpsk_send_gpsk_4(struct eap_gpsk_data *data, u8 identifier,
				 size_t *respDataLen)
{
	struct eap_hdr *resp;
	u8 *rpos, *start;
	size_t len;

	wpa_printf(MSG_DEBUG, "EAP-GPSK: Sending Response/GPSK-4");

	len = 1 + 2 + eap_gpsk_mic_len(data->vendor, data->specifier);

	resp = eap_msg_alloc(EAP_VENDOR_IETF, EAP_TYPE_GPSK, respDataLen, len,
			     EAP_CODE_RESPONSE, identifier, &rpos);
	if (resp == NULL)
		return NULL;

	*rpos++ = EAP_GPSK_OPCODE_GPSK_4;
	start = rpos;

	/* No PD_Payload_3 */
	WPA_PUT_BE16(rpos, 0);
	rpos += 2;

	if (eap_gpsk_compute_mic(data->sk, data->sk_len, data->vendor,
				 data->specifier, start, rpos - start, rpos) <
	    0) {
		eap_gpsk_state(data, FAILURE);
		os_free(resp);
		return NULL;
	}

	return (u8 *) resp;
}


static u8 * eap_gpsk_process(struct eap_sm *sm, void *priv,
			    struct eap_method_ret *ret,
			    const u8 *reqData, size_t reqDataLen,
			    size_t *respDataLen)
{
	struct eap_gpsk_data *data = priv;
	u8 *resp;
	const u8 *pos;
	size_t len;

	pos = eap_hdr_validate(EAP_VENDOR_IETF, EAP_TYPE_GPSK,
			       reqData, reqDataLen, &len);
	if (pos == NULL || len < 1) {
		ret->ignore = TRUE;
		return NULL;
	}

	wpa_printf(MSG_DEBUG, "EAP-GPSK: Received frame: opcode %d", *pos);

	ret->ignore = FALSE;
	ret->methodState = METHOD_MAY_CONT;
	ret->decision = DECISION_FAIL;
	ret->allowNotifications = FALSE;

	switch (*pos) {
	case EAP_GPSK_OPCODE_GPSK_1:
		resp = eap_gpsk_process_gpsk_1(sm, data, ret, reqData,
					       reqDataLen, pos + 1, len - 1,
					       respDataLen);
		break;
	case EAP_GPSK_OPCODE_GPSK_3:
		resp = eap_gpsk_process_gpsk_3(sm, data, ret, reqData,
					       reqDataLen, pos + 1, len - 1,
					       respDataLen);
		break;
	default:
		wpa_printf(MSG_DEBUG, "EAP-GPSK: Ignoring message with "
			   "unknown opcode %d", *pos);
		ret->ignore = TRUE;
		return NULL;
	}

	return resp;
}


static Boolean eap_gpsk_isKeyAvailable(struct eap_sm *sm, void *priv)
{
	struct eap_gpsk_data *data = priv;
	return data->state == SUCCESS;
}


static u8 * eap_gpsk_getKey(struct eap_sm *sm, void *priv, size_t *len)
{
	struct eap_gpsk_data *data = priv;
	u8 *key;

	if (data->state != SUCCESS)
		return NULL;

	key = os_malloc(EAP_MSK_LEN);
	if (key == NULL)
		return NULL;
	os_memcpy(key, data->msk, EAP_MSK_LEN);
	*len = EAP_MSK_LEN;

	return key;
}


static u8 * eap_gpsk_get_emsk(struct eap_sm *sm, void *priv, size_t *len)
{
	struct eap_gpsk_data *data = priv;
	u8 *key;

	if (data->state != SUCCESS)
		return NULL;

	key = os_malloc(EAP_EMSK_LEN);
	if (key == NULL)
		return NULL;
	os_memcpy(key, data->emsk, EAP_EMSK_LEN);
	*len = EAP_EMSK_LEN;

	return key;
}


int eap_peer_gpsk_register(void)
{
	struct eap_method *eap;
	int ret;

	eap = eap_peer_method_alloc(EAP_PEER_METHOD_INTERFACE_VERSION,
				    EAP_VENDOR_IETF, EAP_TYPE_GPSK, "GPSK");
	if (eap == NULL)
		return -1;

	eap->init = eap_gpsk_init;
	eap->deinit = eap_gpsk_deinit;
	eap->process = eap_gpsk_process;
	eap->isKeyAvailable = eap_gpsk_isKeyAvailable;
	eap->getKey = eap_gpsk_getKey;
	eap->get_emsk = eap_gpsk_get_emsk;

	ret = eap_peer_method_register(eap);
	if (ret)
		eap_peer_method_free(eap);
	return ret;
}
