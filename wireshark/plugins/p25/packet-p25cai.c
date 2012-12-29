/* packet-p25cai.c
 * Routines for APCO Project 25 Common Air Interface dissection
 * Copyright 2008, Michael Ossmann <mike@ossmann.com>
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <epan/packet.h>
#include <epan/prefs.h>

#define FRAME_SYNC_MAGIC 0x5575F5FF77FF

/* function prototypes */
void proto_reg_handoff_p25cai(void);
void dissect_voice(tvbuff_t *tvb, proto_tree *tree, int offset);
void dissect_lc(tvbuff_t *tvb, proto_tree *tree);
void dissect_es(tvbuff_t *tvb, proto_tree *tree);
void dissect_pdu(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *tree);
void dissect_pdu_response(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *tree, guint8 *header, int num_blocks);
void dissect_pdu_unconfirmed(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *tree, guint8 *header, int num_blocks);
void dissect_pdu_confirmed(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *tree, guint8 *header, int num_blocks);
void dissect_pdu_ambt(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *tree, guint8 *header, int num_blocks);
tvbuff_t* extract_status_symbols(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int *outbound);
tvbuff_t* build_hdu_tvb(tvbuff_t *tvb, packet_info *pinfo, int offset);
tvbuff_t* build_tsdu_tvb(tvbuff_t *tvb, packet_info *pinfo, int offset);
tvbuff_t* build_term_lc_tvb(tvbuff_t *tvb, packet_info *pinfo, int offset);
tvbuff_t* build_ldu_lsd_tvb(tvbuff_t *tvb, packet_info *pinfo, int offset);
tvbuff_t* build_ldu_lc_tvb(tvbuff_t *tvb, packet_info *pinfo, int offset);
tvbuff_t* build_ldu_es_tvb(tvbuff_t *tvb, packet_info *pinfo, int offset);
void data_deinterleave(tvbuff_t *tvb, guint8 *deinterleaved, int bit_offset);
void trellis_1_2_decode(guint8 *encoded, guint8 *decoded, int offset);
void trellis_3_4_decode(guint8 *encoded, guint8 *decoded, int offset);
guint8 golay_18_6_8_decode(guint32 codeword);
guint16 golay_24_12_8_decode(guint32 codeword);
guint8 hamming_10_6_3_decode(guint32 codeword);
void cyclic_16_8_5_decode(guint32 codeword, guint8 *decoded);
void rs_24_12_13_decode(guint8 *codeword, guint8 *decoded);
void rs_24_16_9_decode(guint8 *codeword, guint8 *decoded);
void rs_36_20_17_decode(guint8 *codeword, guint8 *decoded);
int find_min(guint8 list[], int len);
int count_bits(unsigned int n);

/* Initialize the protocol and registered fields */
static int proto_p25cai = -1;
static int hf_p25cai_fs = -1;
static int hf_p25cai_nid = -1;
static int hf_p25cai_nac = -1;
static int hf_p25cai_duid = -1;
static int hf_p25cai_hdu = -1;
static int hf_p25cai_tsbk = -1;
static int hf_p25cai_pdu = -1;
static int hf_p25cai_ldu1 = -1;
static int hf_p25cai_ldu2 = -1;
static int hf_p25cai_mi = -1;
static int hf_p25cai_mfid = -1;
static int hf_p25cai_algid = -1;
static int hf_p25cai_kid = -1;
static int hf_p25cai_tgid = -1;
static int hf_p25cai_ss_parent = -1;
static int hf_p25cai_ss = -1;
static int hf_p25cai_lc = -1;
static int hf_p25cai_lcf = -1;
static int hf_p25cai_lbf = -1;
static int hf_p25cai_ptbf = -1;
static int hf_p25cai_isp_opcode = -1;
static int hf_p25cai_osp_opcode = -1;
static int hf_p25cai_unknown_opcode = -1;
static int hf_p25cai_args = -1;
static int hf_p25cai_crc = -1;
static int hf_p25cai_imbe = -1;
static int hf_p25cai_lsd = -1;
static int hf_p25cai_es = -1;
static int hf_p25cai_an = -1;
static int hf_p25cai_io = -1;
static int hf_p25cai_pdu_format = -1;
static int hf_p25cai_sapid = -1;
static int hf_p25cai_llid = -1;
static int hf_p25cai_fmf = -1;
static int hf_p25cai_btf = -1;
static int hf_p25cai_poc = -1;
static int hf_p25cai_syn = -1;
static int hf_p25cai_ns = -1;
static int hf_p25cai_fsnf = -1;
static int hf_p25cai_dho = -1;
static int hf_p25cai_db = -1;
static int hf_p25cai_dbsn = -1;
static int hf_p25cai_crc9 = -1;
static int hf_p25cai_ud = -1;
static int hf_p25cai_packet_crc = -1;
static int hf_p25cai_class = -1;
static int hf_p25cai_type = -1;
static int hf_p25cai_status = -1;
static int hf_p25cai_x = -1;
static int hf_p25cai_sllid = -1;

/* Field values */
static const value_string data_unit_ids[] = {
	{ 0x0, "Header Data Unit" },
	{ 0x3, "Terminator without Link Control" },
	{ 0x5, "Logical Link Data Unit 1" },
	{ 0x7, "Trunking Signaling Data Unit" },
	{ 0xA, "Logical Link Data Unit 2" },
	{ 0xC, "Packet Data Unit" },
	{ 0xF, "Terminator with Link Control" },
	{ 0, NULL }
};

static const value_string network_access_codes[] = {
	{ 0x293, "Default NAC" },
	{ 0xF7E, "Receiver to open on any NAC" },
	{ 0xF7F, "Repeater to receive and retransmit any NAC" },
	{ 0, NULL }
};

static const value_string manufacturer_ids[] = {
	/* from http://ftp.tiaonline.org/TR-8/TR815/Public/MFID_Assignments_080723.doc */
	{ 0x00, "Standard MFID (pre-2001)" },
	{ 0x01, "Standard MFID (post-2001)" },
	{ 0x09, "Aselsan Inc." },
	{ 0x10, "Relm / BK Radio" },
	{ 0x18, "EADS Public Safety Inc." },
	{ 0x20, "Cycomm" },
	{ 0x28, "Efratom Time and Frequency Products, Inc" },
	{ 0x30, "Com-Net Ericsson" },
	{ 0x34, "Etherstack" },
	{ 0x38, "Datron" },
	{ 0x40, "Icom" },
	{ 0x48, "Garmin" },
	{ 0x50, "GTE" },
	{ 0x55, "IFR Systems" },
	{ 0x5A, "INIT Innovations in Transportation, Inc" },
	{ 0x60, "GEC-Marconi" },
	{ 0x64, "Harris Corp." },
	{ 0x68, "Kenwood Communications" },
	{ 0x70, "Glenayre Electronics" },
	{ 0x74, "Japan Radio Co." },
	{ 0x78, "Kokusai" },
	{ 0x7C, "Maxon" },
	{ 0x80, "Midland" },
	{ 0x86, "Daniels Electronics Ltd." },
	{ 0x90, "Motorola" },
	{ 0xA0, "Thales" },
	{ 0xA4, "M/A-COM" },
	{ 0xB0, "Raytheon" },
	{ 0xC0, "SEA" },
	{ 0xC8, "Securicor" },
	{ 0xD0, "ADI" },
	{ 0xD8, "Tait Electronics" },
	{ 0xE0, "Teletec" },
	{ 0xF0, "Transcrypt International" },
	{ 0xF8, "Vertex Standard" },
	{ 0xFC, "Zetron, Inc" },
	{ 0, NULL }
};

/* TODO: use link_control_opcodes instead */
static const value_string link_control_formats[] = {
	{ 0x00, "Group Call Format" },
	{ 0x03, "Individual Call Format" },
	{ 0x80, "Encrypted Group Call Format" },
	{ 0x83, "Encrypted Individual Call Format" },
	{ 0, NULL }
};

static const value_string link_control_opcodes[] = {
	/* from AABF */
	{ 0x00, "Group Voice Channel User (LCGVR)" }, /* LC_GRP_V_CH_USR */
	{ 0x01, "Reserved" },
	{ 0x02, "Group Voice Channel Update (LCGVU)" }, /* LC_GRP_V_CH_UPDT */
	{ 0x03, "Unit to Unit Voice Channel User (LCUVR)" }, /* LC_UU_V_CH_USR */
	{ 0x04, "Group Voice Channel Update - Explicit (LCGVUX)" }, /* LC_GRP_CH_UPDT_EXP */
	{ 0x05, "Unit to Unit Answer Request (LCUAQ)" }, /* LC_UU_ANS_REQ */
	{ 0x06, "Telephone Interconnect Voice Channel User (LCTVR)" }, /* LC_TELE_INT_V_CH_USR */
	{ 0x07, "Telephone Interconnect Answer Request (LCTAQ)" }, /* LC_TELE_INT_ANS_REQ */
	{ 0x08, "Reserved" },
	{ 0x09, "Reserved" },
	{ 0x0A, "Reserved" },
	{ 0x0B, "Reserved" },
	{ 0x0C, "Reserved" },
	{ 0x0D, "Reserved" },
	{ 0x0E, "Reserved" },
	{ 0x0F, "Call Termination/Cancellation (LCCT)" }, /* LC_CALL_TRM_CAN */
	{ 0x10, "Group Affiliation Query (LCGAQ)" }, /* LC_GRP_AFF_Q */
	{ 0x11, "Unit Registration Command (LCRC)" }, /* LC_U_REG_CMD */
	{ 0x12, "Unit Authentication Command (LCAC)" }, /* LC_AUTH_CMD */
	{ 0x13, "Status Query (LCSQ)" }, /* LC_STS_Q */
	{ 0x14, "Status Update (LCSU)" }, /* LC_STA_UPDT */
	{ 0x15, "Message Update (LCMU)" }, /* LC_MSG_UPDT */
	{ 0x16, "Call Alert (LCCA)" }, /* LC_CALL_ALRT */
	{ 0x17, "Extended Function Command (LCEFC)" }, /* LC_EXT_FNCT_CMD */
	{ 0x18, "Channel Identifier Update (LCCIU)" }, /* LC_CH_ID_UPDT */
	{ 0x19, "Channel Identifier Update - Explicit (LCCIUX)" }, /* LC_CH_ID_UPDT_EXP */
	{ 0x1A, "Reserved" },
	{ 0x1B, "Reserved" },
	{ 0x1C, "Reserved" },
	{ 0x1D, "Reserved" },
	{ 0x1E, "Reserved" },
	{ 0x1F, "Reserved" },
	{ 0x20, "System Service Broadcast (LCSSB)" }, /* LC_SYS_SRV_BCST */
	{ 0x21, "Secondary Control Channel Broadcast (LCSCB)" }, /* LC_SCCB */
	{ 0x22, "Adjacent Site Status Broadcast (LCASB)" }, /* LC_ADJ_STS_BCST */
	{ 0x23, "RFSS Status Broadcast (LCRSB)" }, /* LC_RFSS_STS_BCST */
	{ 0x24, "Network Status Broadcast (LCNSB)" }, /* LC_NET_STS_BCST */
	{ 0x25, "Protection Parameter Broadcast (LCPPB)" }, /* LC_P_PARM_BCST */
	{ 0x26, "Secondary Control Channel Broadcast - Explicit (LCSCBX)" }, /* LC_SCCB_EXP */
	{ 0x27, "Adjacent Site Status Broadcast - Explicit (LCASBX)" }, /* LC_ADJ_STS_BCST_EXP */
	{ 0x28, "RFSS Status Broadcast - Explicit (LCRSBX)" }, /* LC_RFSS_STS_BCST_EXP */
	{ 0x29, "Network Status Broadcast - Explicit (LCNSBX)" }, /* LC_NET_STS_BCST_EXP */
	{ 0x2A, "Reserved" },
	{ 0x2B, "Reserved" },
	{ 0x2C, "Reserved" },
	{ 0x2D, "Reserved" },
	{ 0x2E, "Reserved" },
	{ 0x2F, "Reserved" },
	{ 0x30, "Reserved" },
	{ 0x31, "Reserved" },
	{ 0x32, "Reserved" },
	{ 0x33, "Reserved" },
	{ 0x34, "Reserved" },
	{ 0x35, "Reserved" },
	{ 0x36, "Reserved" },
	{ 0x37, "Reserved" },
	{ 0x38, "Reserved" },
	{ 0x39, "Reserved" },
	{ 0x3A, "Reserved" },
	{ 0x3B, "Reserved" },
	{ 0x3C, "Reserved" },
	{ 0x3D, "Reserved" },
	{ 0x3E, "Reserved" },
	{ 0x3F, "Reserved" },
	{ 0, NULL }
};

static const range_string logical_link_ids[] = {
	{ 0x000000, 0x000000, "No One" },
	{ 0x000001, 0x98967F, "General Use" },
	{ 0x989680, 0xFFFFFE, "Talk Group or Special Purpose Use" },
	{ 0xFFFFFF, 0xFFFFFF, "Everyone" },
	{ 0, 0, NULL }
};

static const value_string talk_group_ids[] = {
	{ 0x0000, "No One" },
	{ 0x0001, "Default Talk Group" },
	{ 0xFFFF, "Everyone" },
	{ 0, NULL }
};

static const value_string key_ids[] = {
	{ 0x0000, "Default Key ID" },
	{ 0, NULL }
};

static const value_string algorithm_ids[] = {
	/* mostly from http://ftp.tiaonline.org/TR-8/TR815/Public/ALGID_Guide_040528.doc */
	/* Type I */
	{ 0x00, "ACCORDION 1.3" },
	{ 0x01, "BATON (Auto Even)" },
	{ 0x02, "FIREFLY Type 1" },
	{ 0x03, "MAYFLY Type 1" },
	{ 0x04, "SAVILLE" },
	{ 0x41, "BATON (Auto Odd)" },
	/* Type III */
	{ 0x80, "Unencrypted message" },
	{ 0x81, "DES-OFB" },
	{ 0x82, "2 key Triple DES" },
	{ 0x83, "3 key Triple DES" },
	{ 0x84, "AES-256" },
	/* Motorola proprietary */
	/* FIXME maybe these should only be interpreted this way if mfid == 0x90 */
	{ 0x9F, "DES-XL" },
	{ 0xA0, "DVI-XL" },
	{ 0xA1, "DVP-XL" },
	{ 0, NULL }
};

static const value_string service_access_points[] = {
	{ 0x00, "Unencrypted User Data" },
	{ 0x01, "Encrypted User Data" },
	{ 0x02, "Circuit Data" },
	{ 0x03, "Circuit Data Control" },
	{ 0x04, "Packet Data" },
	{ 0x05, "Address Resolution Protocol" },
	{ 0x06, "SNDCP Packet Data Control" },
	{ 0x1F, "Extended Address" },
	{ 0x20, "Registration and Authorization" },
	{ 0x21, "Channel Reassignment" },
	{ 0x22, "System Configuration" },
	{ 0x23, "MR Loop-Back" },
	{ 0x24, "MR Statistics" },
	{ 0x25, "MR Out-of-Service" },
	{ 0x26, "MR Paging" },
	{ 0x27, "MR Configuration" },
	{ 0x28, "Unencrypted Key Management Message" },
	{ 0x29, "Encrypted Key Management Message" },
	{ 0x3D, "Trunking Control" },
	{ 0x3F, "Protected Trunking Control" },
	{ 0, NULL }
};

static const value_string emergency_indicators[] = {
	{ 0x0, "Routine, non-emergency condition" },
	{ 0x1, "Emergency condition" },
	{ 0, NULL }
};

static const value_string isp_opcodes[] = {
	/* voice service isp */
	/* TODO: dissect AABC page 43 */
	{ 0x00, "Group Voice Service Request" }, /* GRP_V_REQ */
	{ 0x04, "Unit To Unit Voice Service Request" }, /* UU_V_REQ */
	{ 0x05, "Unit To Unit Answer Response" }, /* UU_ANS_RSP */
	{ 0x08, "Telephone Interconnect Request - Explicit Dialing" }, /* TELE_INT_DIAL_REQ */
	{ 0x09, "Telephone Interconnect Request - Implicit Dialing" }, /* TELE_INT_PSTN_REQ */
	{ 0x0A, "Telephone Interconnect Answer Response" }, /* TELE_INT_ANS_RSP */

	/* data service isp */
	/* TODO: dissect AABC page 69 */
	{ 0x10, "Individual Data Service Request (obsolete)" }, /* IND_DATA_REQ */
	{ 0x11, "Group Data Service Request (obsolete)" }, /* GRP_DATA_REQ */
	{ 0x12, "SNDCP Data Channel Request" }, /* SN-DAT_CHN_REQ */
	{ 0x13, "SNDCP Data Page Response" }, /* SN-DAT_PAGE_RES */
	{ 0x14, "SNDCP Reconnect Request" }, /* SN-REC_REQ */

	/* control and status isp */
	/* TODO: dissect AABC page 86 */
	{ 0x20, "Acknowledge Response - Unit" }, /* ACK_RSP_U */
	{ 0x2E, "Authentication Query" }, /* AUTH_Q */
	{ 0x2F, "Authentication Response" }, /* AUTH_RSP */
	{ 0x1F, "Call Alert Request" }, /* CALL_ALRT_REQ */
	{ 0x23, "Cancel Service Request" }, /* CAN_SRV_REQ */
	{ 0x27, "Emergency Alarm Request" }, /* EMRG_ALRM_REQ */
	{ 0x24, "Extended Function Response" }, /* EXT_FNCT_RSP */
	{ 0x29, "Group Affiliation Query Response" }, /* GRP_AFF_Q_RSP */
	{ 0x28, "Group Affiliation Request" }, /* GRP_AFF_REQ */
	{ 0x32, "Identifier Update Request" }, /* IDEN_UP_REQ */
	{ 0x1C, "Message Update Request" }, /* MSG_UPDT_REQ */
	{ 0x30, "Protection Parameter Request" }, /* P_PARM_REQ */
	{ 0x1A, "Status Query Request" }, /* STS_Q_REQ */
	{ 0x19, "Status Query Response" }, /* STS_Q_RSP */
	{ 0x18, "Status Update Request" }, /* STS_UPDT_REQ */
	{ 0x2C, "Unit Registration Request" }, /* U_REG_REQ */
	{ 0x2B, "De-Registration Request" }, /* U_DE_REG_REQ */
	{ 0x2D, "Location Registration Request" }, /* LOC_REG_REQ */
	{ 0x1D, "Radio Unit Monitor Request" }, /* RAD_MON_REQ */
	{ 0x36, "Roaming Address Request" }, /* ROAM_ADDR_REQ */
	{ 0x37, "Roaming Address Response" }, /* ROAM_ADDR_RSP */

	{ 0, NULL }
};

static const value_string osp_opcodes[] = {
	/* voice service osp */
	/* TODO: dissect AABC page 53 */
	{ 0x00, "Group Voice Channel Grant" }, /* GRP_V_CH_GRANT */
	{ 0x02, "Group Voice Channel Grant Update" }, /* GRP_V_CH_GRANT_UPDT */
	{ 0x03, "Group Voice Channel Grant Update - Explicit" }, /* GRP_V_CH_GRANT_UPDT_EXP */
	{ 0x04, "Unit To Unit Voice Channel Grant" }, /* UU_V_CH_GRANT */
	{ 0x05, "Unit To Unit Answer Request" }, /* UU_ANS_REQ */
	{ 0x06, "Unit To Unit Voice Channel Grant Update" }, /* UU_V_CH_GRANT_UPDT */
	{ 0x08, "Telephone Interconnect Voice Channel Grant" }, /* TELE_INT_CH_GRANT */
	{ 0x09, "Telephone Interconnect Voice Channel Grant Update" }, /* TELE_INT_CH_GRANT_UPDT */
	{ 0x0A, "Telephone Interconnect Answer Request" }, /* TELE_INT_ANS_REQ */

	/* data service osp */
	/* TODO: dissect AABC page 76 */
	{ 0x10, "Individual Data Channel Grant (obsolete)" }, /* IND_DATA_CH_GRANT */
	{ 0x11, "Group Data Channel Grant (obsolete)" }, /* GRP_DATA_CH_GRANT */
	{ 0x12, "Group Data Channel Announcement (obsolete)" }, /* GRP_DATA_CH_ANN */
	{ 0x13, "Group Data Channel Announcement Explicit (obsolete)" }, /* GRP_DATA_CH_ANN_EXP */
	{ 0x14, "SNDCP Data Channel Grant" }, /* SN-DATA_CHN_GNT */
	{ 0x15, "SNDCP Data Page Request" }, /* SN-DATA_PAGE_REQ */
	{ 0x16, "SNDCP Data Channel Announcement - Explicit" }, /* SN-DAT_CHN_ANN_EXP */

	/* control and status osp */
	/* TODO: dissect AABC page 121 */
	{ 0x20, "Acknowledge Response - FNE" }, /* ACK_RSP_FNE */
	{ 0x3C, "Adjacent Status Broadcast" }, /* ADJ_STS_BCST */
	{ 0x2E, "Authentication Command" }, /* AUTH_CMD */
	{ 0x1F, "Call Alert" }, /* CALL_ALRT */
	{ 0x27, "Deny Response" }, /* DENY_RSP */
	{ 0x24, "Extended Function Command" }, /* EXT_FNCT_CMD */
	{ 0x2A, "Group Affiliation Query" }, /* GRP_AFF_Q */
	{ 0x28, "Group Affiliation Response" }, /* GRP_AFF_RSP */
	{ 0x3D, "Identifier Update" }, /* IDEN_UP */
	{ 0x1C, "Message Update" }, /* MSG_UPDT */
	{ 0x3B, "Network Status Broadcast" }, /* NET_STS_BCST */
	{ 0x3E, "Protection Parameter Broadcast" }, /* P_PARM_BCST */
	{ 0x3F, "Protection Parameter Update" }, /* P_PARM_UPDT */
	{ 0x21, "Queued Response" }, /* QUE_RSP */
	{ 0x3A, "RFSS Status Broadcast" }, /* RFSS_STS_BCST */
	{ 0x39, "Secondary Control Channel Broadcast" }, /* SCCB */
	{ 0x1A, "Status Query" }, /* STS_Q */
	{ 0x18, "Status Update" }, /* STS_UPDT */
	{ 0x38, "System Service Broadcast" }, /* SYS_SRV_BCST */
	{ 0x2D, "Unit Registration Command" }, /* U_REG_CMD */
	{ 0x2C, "Unit Registration Response" }, /* U_REG_RSP */
	{ 0x2F, "De-Registration Acknowledge" }, /* U_DE_REG_ACK */
	{ 0x2B, "Location Registration Response" }, /* LOC_REG_RSP */
	{ 0x1D, "Radio Unit Monitor Command" }, /* RAD_MON_CMD */
	{ 0x36, "Roaming Address Command" }, /* ROAM_ADDR_CMD */
	{ 0x37, "Roaming Address Update" }, /* ROAM_ADDR_UPDT */
	{ 0x35, "Time and Date Announcement" }, /* TIME_DATE_ANN */
	{ 0x34, "Identifier Update for VHF/UHF Bands" }, /* IDEN_UP_VU */
	{ 0x29, "Secondary Control Channel Broadcast - Explicit" }, /* SCCB_EXP */

	{ 0, NULL }
};

static const range_string cancel_reason_codes[] = {
	{ 0x00, 0x00, "No Reason Code" },
	{ 0x01, 0x0F, "Reserved" },
	{ 0x10, 0x10, "Terminate Queued Condition" },
	{ 0x11, 0x1F, "Reserved" },
	{ 0x20, 0x20, "Terminate Resource Assignment" },
	{ 0x21, 0x7F, "Reserved" },
	{ 0x80, 0xFF, "User or System Definable" },
	{ 0, 0, NULL }
};

static const range_string deny_response_reason_codes[] = {
	{ 0x00, 0x0F, "Reserved" },
	{ 0x10, 0x10, "The requestion unit is not valid" },
	{ 0x11, 0x11, "The requestion unit is not authorized for this service" },
	{ 0x12, 0x1F, "Reserved" },
	{ 0x20, 0x20, "The target unit is not valid" },
	{ 0x21, 0x21, "The target unit is not authorized for this service" },
	{ 0x22, 0x2E, "Reserved" },
	{ 0x2F, 0x2F, "Target unit has refused this call" },
	{ 0x30, 0x30, "The target group is not valid" },
	{ 0x31, 0x31, "The target group is not authorized for this service" },
	{ 0x32, 0x3F, "Reserved" },
	{ 0x40, 0x40, "Invalid dialing" },
	{ 0x41, 0x41, "Telephone number is not authorized" },
	{ 0x42, 0x42, "PSTN address is not valid" },
	{ 0x43, 0x4F, "Reserved" },
	{ 0x50, 0x50, "Call time-out has occurred" },
	{ 0x51, 0x51, "Landline has terminated this call" },
	{ 0x52, 0x52, "Subscriber unit has terminated this call" },
	{ 0x53, 0x5E, "Reserved" },
	{ 0x5F, 0x5F, "Call has been pre-empted" },
	{ 0x60, 0x60, "Site access denial" },
	{ 0x61, 0xEF, "User or system definable" },
	{ 0xF0, 0xF0, "The call options are not valid for thie service" },
	{ 0xF1, 0xF1, "Protection service option is not valid" },
	{ 0xF2, 0xF2, "Duplex service option is not valid" },
	{ 0xF3, 0xF3, "Circuit or packet mode service option is not valid" },
	{ 0xF4, 0xFE, "User or system definable" },
	{ 0xFF, 0xFF, "The system does not support this service" },
	{ 0, 0, NULL }
};

static const range_string queued_response_reason_codes[] = {
	{ 0x00, 0x0F, "Reserved" },
	{ 0x10, 0x10, "The requesting unit is active in another service" },
	{ 0x11, 0x1F, "Reserved" },
	{ 0x10, 0x10, "The target unit is active in another service" },
	{ 0x21, 0x2E, "Reserved" },
	{ 0x2F, 0x2F, "The target unit has queued the call" },
	{ 0x30, 0x30, "The target group is currently active" },
	{ 0x31, 0x3F, "Reserved" },
	{ 0x40, 0x40, "Channel resources are not currently active" },
	{ 0x41, 0x41, "Telephone resources are not currently active" },
	{ 0x42, 0x42, "Data resources are not currently active" },
	{ 0x43, 0x4F, "Reserved" },
	{ 0x50, 0x50, "Superseding service currently active" },
	{ 0x51, 0x7F, "Reserved" },
	{ 0x80, 0xFF, "User or System Definable" },
	{ 0, 0, NULL }
};

static const value_string pdu_formats[] = {
	{ 0x03, "Response Packet" },
	{ 0x15, "Unconfirmed Data Packet" },
	{ 0x16, "Confirmed Data Packet" },
	{ 0x17, "Alternate Multiple Block Trunking Control Packet" },
	{ 0, NULL }
};

static const true_false_string inbound_outbound = {
	"Outbound",
	"Inbound"
};

static const value_string status_symbols[] = {
	{ 0x0, "Unknown" },
	{ 0x1, "Busy" },
	{ 0x2, "Unknown" },
	{ 0x3, "Idle (start of inbound slot)" },
	{ 0, NULL }
};

/* Initialize the subtree pointers */
static gint ett_p25cai = -1;
static gint ett_ss = -1;
static gint ett_nid = -1;
static gint ett_du = -1;
static gint ett_lc = -1;
static gint ett_es = -1;
static gint ett_db = -1;

/* Code to actually dissect the packets */
static int
dissect_p25cai(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{

/* Set up structures needed to add the protocol subtree and manage it */
	proto_item *ti, *nid_item, *du_item;
	proto_tree *p25cai_tree, *nid_tree, *du_tree;
	int offset, outbound;
	tvbuff_t *extracted_tvb, *du_tvb;
	guint8 duid, last_block, mfid;

/*  If this doesn't look like a P25 CAI frame, give up and return 0 so that
 *  perhaps another dissector can take over.
 */

	/* Check that there's enough data */
	if (tvb_length(tvb) < 14) /* P25 CAI smallest packet size */
		return 0;

	/* Check for correct Frame Sync value
	 *
	 * This is commented out for now so that we can deal with symbol errors.
	 * We could use it if we have error correction working before frames get
	 * passed to wireshark.  Perhaps a more generally useful alternative would
	 * look for a minimum number of matching bits rather than requiring a 100%
	 * match.
	 *
	 * guint64 fs;
	 * fs = tvb_get_ntoh64(tvb, 0) >> 16;
	 * if (fs != FRAME_SYNC_MAGIC)
	 * 	return 0;
	 */

/* Make entries in Protocol column and Info column on summary display */
	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "P25 CAI");

	/* Clear the Info column first just in case of duid fetching failure. */
	if (check_col(pinfo->cinfo, COL_INFO))
		col_clear(pinfo->cinfo, COL_INFO);

	duid = tvb_get_guint8(tvb, 7) & 0xF;

	if (check_col(pinfo->cinfo, COL_INFO))
		col_set_str(pinfo->cinfo, COL_INFO, val_to_str(duid, data_unit_ids, "Unknown Data Unit (0x%02x)"));

	/* see if we are being asked for details */
	if (tree) {

/* create display subtree for the protocol */
		ti = proto_tree_add_item(tree, proto_p25cai, tvb, 0, -1, FALSE);
		p25cai_tree = proto_item_add_subtree(ti, ett_p25cai);
		offset = 0;

		/* top level P25 CAI tree */
		proto_tree_add_item(p25cai_tree, hf_p25cai_fs, tvb, offset, 6, FALSE);
		offset += 6;

		/* Extract Status Symbols */
		extracted_tvb = extract_status_symbols(tvb, pinfo, p25cai_tree, &outbound);

		/* process extracted_tvb from here on */

		nid_item = proto_tree_add_item(p25cai_tree, hf_p25cai_nid, extracted_tvb, offset, 8, FALSE);

		/* NID subtree */
		nid_tree = proto_item_add_subtree(nid_item, ett_nid);
		proto_tree_add_item(nid_tree, hf_p25cai_nac, extracted_tvb, offset, 2, FALSE);
		proto_tree_add_item(nid_tree, hf_p25cai_duid, extracted_tvb, offset, 2, FALSE);
		offset += 8;

		switch (duid) {
		/* Header Data Unit */
		case 0x0:
			du_tvb = build_hdu_tvb(extracted_tvb, pinfo, offset);
			offset = 0;
			du_item = proto_tree_add_item(p25cai_tree, hf_p25cai_hdu, du_tvb, offset, -1, FALSE);
			du_tree = proto_item_add_subtree(du_item, ett_du);
			proto_tree_add_item(du_tree, hf_p25cai_mi, du_tvb, offset, 9, FALSE);
			offset += 9;
			proto_tree_add_item(du_tree, hf_p25cai_mfid, du_tvb, offset, 1, FALSE);
			offset += 1;
			proto_tree_add_item(du_tree, hf_p25cai_algid, du_tvb, offset, 1, FALSE);
			offset += 1;
			proto_tree_add_item(du_tree, hf_p25cai_kid, du_tvb, offset, 2, FALSE);
			offset += 2;
			proto_tree_add_item(du_tree, hf_p25cai_tgid, du_tvb, offset, 2, FALSE);
			offset += 2;
			break;
		/* Terminator Data Unit without Link Control */
		case 0x3:
			/* nothing left to decode */
			break;
		/* Logical Link Data Unit 1 */
		case 0x5:
			du_item = proto_tree_add_item(p25cai_tree, hf_p25cai_ldu1, extracted_tvb, offset, -1, FALSE);
			du_tree = proto_item_add_subtree(du_item, ett_du);
			dissect_voice(extracted_tvb, du_tree, offset);
			dissect_lc(build_ldu_lc_tvb(extracted_tvb, pinfo, offset), du_tree);
			proto_tree_add_item(du_tree, hf_p25cai_lsd, build_ldu_lsd_tvb(extracted_tvb, pinfo, offset), 0, 2, FALSE);
			break;
		/* Trunking Signaling Data Unit */
		case 0x7:
			du_tvb = build_tsdu_tvb(extracted_tvb, pinfo, offset);
			offset = 0;
			last_block = 0;
			while (last_block == 0) {
				last_block = tvb_get_guint8(du_tvb, offset) >> 7;
				mfid = tvb_get_guint8(du_tvb, offset + 1);
				du_item = proto_tree_add_item(p25cai_tree, hf_p25cai_tsbk, du_tvb, offset, 12, FALSE);
				du_tree = proto_item_add_subtree(du_item, ett_du);
				proto_tree_add_item(du_tree, hf_p25cai_lbf, du_tvb, offset, 1, FALSE);
				proto_tree_add_item(du_tree, hf_p25cai_ptbf, du_tvb, offset, 1, FALSE);
				if (mfid > 1) {
					proto_tree_add_item(du_tree, hf_p25cai_unknown_opcode, du_tvb, offset, 1, FALSE);
				} else if (outbound) {
					proto_tree_add_item(du_tree, hf_p25cai_osp_opcode, du_tvb, offset, 1, FALSE);
				} else {
					proto_tree_add_item(du_tree, hf_p25cai_isp_opcode, du_tvb, offset, 1, FALSE);
				}
				offset += 1;
				proto_tree_add_item(du_tree, hf_p25cai_mfid, du_tvb, offset, 1, FALSE);
				offset += 1;
				proto_tree_add_item(du_tree, hf_p25cai_args, du_tvb, offset, 8, FALSE);
				offset += 8;
				/* TODO: dissect args subtree */
				proto_tree_add_item(du_tree, hf_p25cai_crc, du_tvb, offset, 2, FALSE);
				/* TODO: verify CRC */
				offset += 2;
			}
			break;
		/* Logical Link Data Unit 2 */
		case 0xA:
			du_item = proto_tree_add_item(p25cai_tree, hf_p25cai_ldu2, extracted_tvb, offset, -1, FALSE);
			du_tree = proto_item_add_subtree(du_item, ett_du);
			dissect_voice(extracted_tvb, du_tree, offset);
			dissect_es(build_ldu_es_tvb(extracted_tvb, pinfo, offset), du_tree);
			proto_tree_add_item(du_tree, hf_p25cai_lsd, build_ldu_lsd_tvb(extracted_tvb, pinfo, offset), 0, 2, FALSE);
			break;
		/* Packet Data Unit */
		/* TODO: This case is hasn't been tested. */
		case 0xC:
			dissect_pdu(extracted_tvb, pinfo, offset, p25cai_tree);
			break;
		/* Terminator Data Unit with Link Control */
		case 0xF:
			dissect_lc(build_term_lc_tvb(extracted_tvb, pinfo, offset), p25cai_tree);
			break;
		/* Unknown Data Unit */
		default:
			/* don't know how to decode any more */
			break;
		}

	}

/* Return the amount of data this dissector was able to dissect */
	return tvb_length(tvb);
}

/* Dissect voice frames */
void
dissect_voice(tvbuff_t *tvb, proto_tree *tree, int offset)
{
	DISSECTOR_ASSERT(tvb_length_remaining(tvb, offset) >= 196);


	/* TODO: make these less raw */
	proto_tree_add_item(tree, hf_p25cai_imbe, tvb, offset, 18, FALSE);
	offset += 18;
	proto_tree_add_item(tree, hf_p25cai_imbe, tvb, offset, 18, FALSE);
	offset += 23;
	proto_tree_add_item(tree, hf_p25cai_imbe, tvb, offset, 18, FALSE);
	offset += 23;
	proto_tree_add_item(tree, hf_p25cai_imbe, tvb, offset, 18, FALSE);
	offset += 23;
	proto_tree_add_item(tree, hf_p25cai_imbe, tvb, offset, 18, FALSE);
	offset += 23;
	proto_tree_add_item(tree, hf_p25cai_imbe, tvb, offset, 18, FALSE);
	offset += 23;
	proto_tree_add_item(tree, hf_p25cai_imbe, tvb, offset, 18, FALSE);
	offset += 23;
	proto_tree_add_item(tree, hf_p25cai_imbe, tvb, offset, 18, FALSE);
	offset += 22;
	proto_tree_add_item(tree, hf_p25cai_imbe, tvb, offset, 18, FALSE);
	offset += 18;
}

/* Dissect Link Control */
void
dissect_lc(tvbuff_t *tvb, proto_tree *tree)
{
	proto_tree *lc_tree;
	proto_item *lc_item;
	int offset = 0;

	DISSECTOR_ASSERT(tvb_length_remaining(tvb, offset) >= 9);

	lc_item = proto_tree_add_item(tree, hf_p25cai_lc, tvb, offset, 9, FALSE);
	lc_tree = proto_item_add_subtree(lc_item, ett_lc);
	proto_tree_add_item(lc_tree, hf_p25cai_lcf, tvb, offset, 1, FALSE);
	offset += 1;
	/* TODO: Decode Link Control according to Link Control Format. */
}

/* Dissect Encryption Sync from an LDU2 */
void
dissect_es(tvbuff_t *tvb, proto_tree *tree)
{
	proto_tree *es_tree;
	proto_item *es_item;
	int offset = 0;

	DISSECTOR_ASSERT(tvb_length_remaining(tvb, offset) >= 12);

	es_item = proto_tree_add_item(tree, hf_p25cai_es, tvb, offset, 12, FALSE);
	es_tree = proto_item_add_subtree(es_item, ett_es);
	proto_tree_add_item(es_tree, hf_p25cai_mi, tvb, offset, 9, FALSE);
	offset += 9;
	proto_tree_add_item(es_tree, hf_p25cai_algid, tvb, offset, 1, FALSE);
	offset += 1;
	proto_tree_add_item(es_tree, hf_p25cai_kid, tvb, offset, 2, FALSE);
}

/* Dissect Packet Data Unit */
void
dissect_pdu(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *tree)
{
	proto_tree *pdu_tree;
	proto_item *pdu_item;
	guint8 *header_buffer, *trellis_buffer, pdu_format;
	int blocks_to_follow;

	pdu_item = proto_tree_add_item(tree, hf_p25cai_pdu, tvb, offset, -1, FALSE);
	pdu_tree = proto_item_add_subtree(pdu_item, ett_du);

	/* Prepare the header portion of the tvb. */
	DISSECTOR_ASSERT(tvb_length_remaining(tvb, offset) >= 25);
	header_buffer = (guint8*)ep_alloc0(12);
	trellis_buffer = (guint8*)ep_alloc0(25);
	data_deinterleave(tvb, trellis_buffer, offset * 8);
	trellis_1_2_decode(trellis_buffer, header_buffer, 0);

	/* Find out how many data blocks we are supposed to have. */
	blocks_to_follow = header_buffer[6] & 0x7F;

	pdu_format = header_buffer[0] & 0x1F;

	switch (pdu_format) {
	/* Response Packet */
	case 0x03:
		dissect_pdu_response(tvb, pinfo, 0, pdu_tree, header_buffer, blocks_to_follow);
		break;
	/* Unconfirmed Data Packet */
	case 0x15:
		dissect_pdu_unconfirmed(tvb, pinfo, 0, pdu_tree, header_buffer, blocks_to_follow);
		break;
	/* Confirmed Data Packet */
	case 0x16:
		dissect_pdu_confirmed(tvb, pinfo, 0, pdu_tree, header_buffer, blocks_to_follow);
		break;
	/* Alternate Multiple Block Trunking (MBT) Control Packet */
	case 0x17:
		dissect_pdu_ambt(tvb, pinfo, 0, pdu_tree, header_buffer, blocks_to_follow);
		break;
	/* Unknown PDU format */
	default:
		break;
	}
}

/* Dissect Response Packet */
void
dissect_pdu_response(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *tree, guint8 *header, int num_blocks)
{
	tvbuff_t *pdu_tvb;
	proto_tree *db_tree;
	proto_item *db_item;
	guint8 *pdu_buffer, *trellis_buffer;
	int i, tvb_bit_offset, pdu_offset, pdu_buffer_length;

	pdu_buffer_length = 12 + num_blocks * 12;
	pdu_buffer = (guint8*)ep_alloc0(pdu_buffer_length);

	/* Our tvb offset doesn't fall on bit boundaries, so we track it by bits
	 * instead of bytes.
	 */
	tvb_bit_offset = 308;
	pdu_offset = 12;

	/* Copy the temporary header onto the pdu_buffer. */
	for (i = 0; i < 12; i++)
		pdu_buffer[i] = header[i];

	/* Add the data blocks to the pdu_buffer. */
	for (i = 0; i < num_blocks; i++) {
		DISSECTOR_ASSERT(tvb_length_remaining(tvb, tvb_bit_offset / 8) >= 25);
		trellis_buffer = (guint8*)ep_alloc0(25);
		data_deinterleave(tvb, trellis_buffer, tvb_bit_offset);
		trellis_1_2_decode(trellis_buffer, pdu_buffer, pdu_offset);
		tvb_bit_offset += 196;
		pdu_offset += 12;
	}

	/* Setup a new tvb buffer with the decoded data. */
	pdu_tvb = tvb_new_real_data(pdu_buffer, pdu_buffer_length, pdu_buffer_length);
	tvb_set_child_real_data_tvbuff(tvb, pdu_tvb);
	add_new_data_source(pinfo, pdu_tvb, "Packet Data Unit");

	/* Dissect the PDU header. */
	proto_tree_add_item(tree, hf_p25cai_io, pdu_tvb, offset, 1, FALSE);
	proto_tree_add_item(tree, hf_p25cai_pdu_format, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	proto_tree_add_item(tree, hf_p25cai_class, pdu_tvb, offset, 1, FALSE);
	proto_tree_add_item(tree, hf_p25cai_type, pdu_tvb, offset, 1, FALSE);
	proto_tree_add_item(tree, hf_p25cai_status, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	proto_tree_add_item(tree, hf_p25cai_mfid, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	proto_tree_add_item(tree, hf_p25cai_llid, pdu_tvb, offset, 3, FALSE);
	offset += 3;
	proto_tree_add_item(tree, hf_p25cai_x, pdu_tvb, offset, 1, FALSE);
	proto_tree_add_item(tree, hf_p25cai_btf, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	proto_tree_add_item(tree, hf_p25cai_sllid, pdu_tvb, offset, 3, FALSE);
	offset += 3;
	proto_tree_add_item(tree, hf_p25cai_crc, pdu_tvb, offset, 2, FALSE);
	offset += 2;

	/* Dissect the data blocks. */
	for (i = 0; i < num_blocks; i++) {
		db_item = proto_tree_add_item(tree, hf_p25cai_db, pdu_tvb, offset, 12, FALSE);
		db_tree = proto_item_add_subtree(db_item, ett_db);
		if (i == num_blocks - 1) {
			/* The last data block includes the packet CRC */
			proto_tree_add_item(db_tree, hf_p25cai_ud, pdu_tvb, offset, 8, FALSE);
			offset += 8;
			proto_tree_add_item(db_tree, hf_p25cai_packet_crc, pdu_tvb, offset, 4, FALSE);
		} else {
			proto_tree_add_item(db_tree, hf_p25cai_ud, pdu_tvb, offset, 12, FALSE);
		}
	}
}

/* Dissect Unonfirmed Data Packet */
void
dissect_pdu_unconfirmed(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *tree, guint8 *header, int num_blocks)
{
	tvbuff_t *pdu_tvb;
	proto_tree *db_tree;
	proto_item *db_item;
	guint8 *pdu_buffer, *trellis_buffer;
	int i, tvb_bit_offset, pdu_offset, pdu_buffer_length;

	pdu_buffer_length = 12 + num_blocks * 12;
	pdu_buffer = (guint8*)ep_alloc0(pdu_buffer_length);

	/* Our tvb offset doesn't fall on bit boundaries, so we track it by bits
	 * instead of bytes.
	 */
	tvb_bit_offset = 308;
	pdu_offset = 12;

	/* Copy the temporary header onto the pdu_buffer. */
	for (i = 0; i < 12; i++)
		pdu_buffer[i] = header[i];

	/* Add the data blocks to the pdu_buffer. */
	for (i = 0; i < num_blocks; i++) {
		DISSECTOR_ASSERT(tvb_length_remaining(tvb, tvb_bit_offset / 8) >= 25);
		trellis_buffer = (guint8*)ep_alloc0(25);
		data_deinterleave(tvb, trellis_buffer, tvb_bit_offset);
		trellis_1_2_decode(trellis_buffer, pdu_buffer, pdu_offset);
		tvb_bit_offset += 196;
		pdu_offset += 12;
	}

	/* Setup a new tvb buffer with the decoded data. */
	pdu_tvb = tvb_new_real_data(pdu_buffer, pdu_buffer_length, pdu_buffer_length);
	tvb_set_child_real_data_tvbuff(tvb, pdu_tvb);
	add_new_data_source(pinfo, pdu_tvb, "Packet Data Unit");

	/* Dissect the PDU header. */
	proto_tree_add_item(tree, hf_p25cai_an, pdu_tvb, offset, 1, FALSE);
	proto_tree_add_item(tree, hf_p25cai_io, pdu_tvb, offset, 1, FALSE);
	proto_tree_add_item(tree, hf_p25cai_pdu_format, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	proto_tree_add_item(tree, hf_p25cai_sapid, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	proto_tree_add_item(tree, hf_p25cai_mfid, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	proto_tree_add_item(tree, hf_p25cai_llid, pdu_tvb, offset, 3, FALSE);
	offset += 3;
	proto_tree_add_item(tree, hf_p25cai_btf, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	proto_tree_add_item(tree, hf_p25cai_poc, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	/* reserved octet here */
	offset += 1;
	proto_tree_add_item(tree, hf_p25cai_dho, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	proto_tree_add_item(tree, hf_p25cai_crc, pdu_tvb, offset, 2, FALSE);
	offset += 2;

	/* Dissect the data blocks. */
	for (i = 0; i < num_blocks; i++) {
		db_item = proto_tree_add_item(tree, hf_p25cai_db, pdu_tvb, offset, 12, FALSE);
		db_tree = proto_item_add_subtree(db_item, ett_db);
		if (i == num_blocks - 1) {
			/* The last data block includes the packet CRC */
			proto_tree_add_item(db_tree, hf_p25cai_ud, pdu_tvb, offset, 8, FALSE);
			offset += 8;
			proto_tree_add_item(db_tree, hf_p25cai_packet_crc, pdu_tvb, offset, 4, FALSE);
		} else {
			proto_tree_add_item(db_tree, hf_p25cai_ud, pdu_tvb, offset, 12, FALSE);
		}
	}
}

/* Dissect Confirmed Data Packet */
void
dissect_pdu_confirmed(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *tree, guint8 *header, int num_blocks)
{
	tvbuff_t *pdu_tvb;
	proto_tree *db_tree;
	proto_item *db_item;
	guint8 *pdu_buffer, *trellis_buffer;
	int i, tvb_bit_offset, pdu_offset, pdu_buffer_length;

	pdu_buffer_length = 12 + num_blocks * 18;
	pdu_buffer = (guint8*)ep_alloc0(pdu_buffer_length);

	/* Our tvb offset doesn't fall on bit boundaries, so we track it by bits
	 * instead of bytes.
	 */
	tvb_bit_offset = 308;
	pdu_offset = 12;

	/* Copy the temporary header onto the pdu_buffer. */
	for (i = 0; i < 12; i++)
		pdu_buffer[i] = header[i];

	/* Add the data blocks to the pdu_buffer. */
	for (i = 0; i < num_blocks; i++) {
		DISSECTOR_ASSERT(tvb_length_remaining(tvb, tvb_bit_offset / 8) >= 25);
		trellis_buffer = (guint8*)ep_alloc0(25);
		data_deinterleave(tvb, trellis_buffer, tvb_bit_offset);
		trellis_3_4_decode(trellis_buffer, pdu_buffer, pdu_offset);
		tvb_bit_offset += 196;
		pdu_offset += 18;
	}

	/* Setup a new tvb buffer with the decoded data. */
	pdu_tvb = tvb_new_real_data(pdu_buffer, pdu_buffer_length, pdu_buffer_length);
	tvb_set_child_real_data_tvbuff(tvb, pdu_tvb);
	add_new_data_source(pinfo, pdu_tvb, "Packet Data Unit");

	/* Dissect the PDU header. */
	proto_tree_add_item(tree, hf_p25cai_an, pdu_tvb, offset, 1, FALSE);
	proto_tree_add_item(tree, hf_p25cai_io, pdu_tvb, offset, 1, FALSE);
	proto_tree_add_item(tree, hf_p25cai_pdu_format, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	proto_tree_add_item(tree, hf_p25cai_sapid, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	proto_tree_add_item(tree, hf_p25cai_mfid, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	proto_tree_add_item(tree, hf_p25cai_llid, pdu_tvb, offset, 3, FALSE);
	offset += 3;
	proto_tree_add_item(tree, hf_p25cai_fmf, pdu_tvb, offset, 1, FALSE);
	proto_tree_add_item(tree, hf_p25cai_btf, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	proto_tree_add_item(tree, hf_p25cai_poc, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	proto_tree_add_item(tree, hf_p25cai_syn, pdu_tvb, offset, 1, FALSE);
	proto_tree_add_item(tree, hf_p25cai_ns, pdu_tvb, offset, 1, FALSE);
	proto_tree_add_item(tree, hf_p25cai_fsnf, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	proto_tree_add_item(tree, hf_p25cai_dho, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	proto_tree_add_item(tree, hf_p25cai_crc, pdu_tvb, offset, 2, FALSE);
	offset += 2;

	/* Dissect the data blocks. */
	for (i = 0; i < num_blocks; i++) {
		db_item = proto_tree_add_item(tree, hf_p25cai_db, pdu_tvb, offset, 18, FALSE);
		db_tree = proto_item_add_subtree(db_item, ett_db);
		proto_tree_add_item(db_tree, hf_p25cai_dbsn, pdu_tvb, offset, 1, FALSE);
		proto_tree_add_item(db_tree, hf_p25cai_crc9, pdu_tvb, offset, 2, FALSE);
		offset += 2;
		if (i == num_blocks - 1) {
			/* The last data block includes the packet CRC */
			proto_tree_add_item(db_tree, hf_p25cai_ud, pdu_tvb, offset, 12, FALSE);
			offset += 12;
			proto_tree_add_item(db_tree, hf_p25cai_packet_crc, pdu_tvb, offset, 4, FALSE);
		} else {
			proto_tree_add_item(db_tree, hf_p25cai_ud, pdu_tvb, offset, 16, FALSE);
		}
	}
}

/* Dissect Alternate Multiple Block Trunking (MBT) Control Packet */
void
dissect_pdu_ambt(tvbuff_t *tvb, packet_info *pinfo, int offset, proto_tree *tree, guint8 *header, int num_blocks)
{
	tvbuff_t *pdu_tvb;
	proto_tree *db_tree;
	proto_item *db_item;
	guint8 *pdu_buffer, *trellis_buffer, outbound, mfid;
	int i, tvb_bit_offset, pdu_offset, pdu_buffer_length;

	pdu_buffer_length = 12 + num_blocks * 12;
	pdu_buffer = (guint8*)ep_alloc0(pdu_buffer_length);

	/* Our tvb offset doesn't fall on bit boundaries, so we track it by bits
	 * instead of bytes.
	 */
	tvb_bit_offset = 308;
	pdu_offset = 12;

	/* Copy the temporary header onto the pdu_buffer. */
	for (i = 0; i < 12; i++)
		pdu_buffer[i] = header[i];

	/* Add the data blocks to the pdu_buffer. */
	for (i = 0; i < num_blocks; i++) {
		DISSECTOR_ASSERT(tvb_length_remaining(tvb, tvb_bit_offset / 8) >= 25);
		trellis_buffer = (guint8*)ep_alloc0(25);
		data_deinterleave(tvb, trellis_buffer, tvb_bit_offset);
		trellis_1_2_decode(trellis_buffer, pdu_buffer, pdu_offset);
		tvb_bit_offset += 196;
		pdu_offset += 12;
	}

	/* Setup a new tvb buffer with the decoded data. */
	pdu_tvb = tvb_new_real_data(pdu_buffer, pdu_buffer_length, pdu_buffer_length);
	tvb_set_child_real_data_tvbuff(tvb, pdu_tvb);
	add_new_data_source(pinfo, pdu_tvb, "Packet Data Unit");

	/* Dissect the PDU header. */
	proto_tree_add_item(tree, hf_p25cai_an, pdu_tvb, offset, 1, FALSE);
	outbound = (tvb_get_guint8(pdu_tvb, offset) >> 5) & 0x01;
	proto_tree_add_item(tree, hf_p25cai_io, pdu_tvb, offset, 1, FALSE);
	proto_tree_add_item(tree, hf_p25cai_pdu_format, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	proto_tree_add_item(tree, hf_p25cai_sapid, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	mfid = tvb_get_guint8(pdu_tvb, offset);
	proto_tree_add_item(tree, hf_p25cai_mfid, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	proto_tree_add_item(tree, hf_p25cai_llid, pdu_tvb, offset, 3, FALSE);
	offset += 3;
	proto_tree_add_item(tree, hf_p25cai_btf, pdu_tvb, offset, 1, FALSE);
	offset += 1;
	if (mfid > 1) {
		proto_tree_add_item(tree, hf_p25cai_unknown_opcode, pdu_tvb, offset, 1, FALSE);
	} else if (outbound) {
		proto_tree_add_item(tree, hf_p25cai_osp_opcode, pdu_tvb, offset, 1, FALSE);
	} else {
		proto_tree_add_item(tree, hf_p25cai_isp_opcode, pdu_tvb, offset, 1, FALSE);
	}
	offset += 1;
	/* TODO: 2 octets defined by trunking messages */
	offset += 2;
	proto_tree_add_item(tree, hf_p25cai_crc, pdu_tvb, offset, 2, FALSE);
	offset += 2;

	/* Dissect the data blocks. */
	for (i = 0; i < num_blocks; i++) {
		db_item = proto_tree_add_item(tree, hf_p25cai_db, pdu_tvb, offset, 12, FALSE);
		db_tree = proto_item_add_subtree(db_item, ett_db);
		if (i == num_blocks - 1) {
			/* The last data block includes the packet CRC */
			/* really "MBT data" not "user data" */
			proto_tree_add_item(db_tree, hf_p25cai_ud, pdu_tvb, offset, 8, FALSE);
			offset += 8;
			proto_tree_add_item(db_tree, hf_p25cai_packet_crc, pdu_tvb, offset, 4, FALSE);
		} else {
			proto_tree_add_item(db_tree, hf_p25cai_ud, pdu_tvb, offset, 12, FALSE);
		}
	}
}

/* Extract status symbols, display them, and return frame without status symbols */
tvbuff_t*
extract_status_symbols(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, int *outbound)
{
	proto_item *ss_parent_item;
	proto_tree *ss_tree;
	int i, j, raw_length, extracted_length;
	guint8 *extracted_buffer;
	tvbuff_t *extracted_tvb;

	raw_length = tvb_length(tvb);
	extracted_length = raw_length - (raw_length / 36);
	*outbound = 0;

	/* Create buffer to become new tvb. */
	extracted_buffer = (guint8*)ep_alloc0(extracted_length);

	/* Create status symbol subtree. */
	ss_parent_item = proto_tree_add_item(tree, hf_p25cai_ss_parent, tvb, 0, -1, FALSE);
	ss_tree = proto_item_add_subtree(ss_parent_item, ett_ss);

	/* Go through frame one dibit at a time. */
	for (i = 0, j = 0; i < raw_length * 4; i++) {
		if (i % 36 == 35) {
			/* After every 35 dibits is a status symbol. */
			proto_tree_add_item(ss_tree, hf_p25cai_ss, tvb, i/4, 1, FALSE);
			/* Check to see if the status symbol is odd. */
			if (tvb_get_guint8(tvb, i/4) & 0x1) {
				/* Flag as outbound (only outbound frames should have odd status symbols).
				 * This may not be a very reliable means of determining the direction, but
				 * I haven't found anything better.
				 */
				*outbound |= 1;
			}
		} else {
			/* Extract frame bits from between status symbols. */
			/* I'm sure there is a more efficient way to do this. */
			extracted_buffer[j/4] |= tvb_get_bits8(tvb, i * 2, 2) << (6 - (j % 4) * 2);
			j++;
		}
	}

	/* Setup a new tvb buffer with the extracted data. */
	extracted_tvb = tvb_new_real_data(extracted_buffer, extracted_length, extracted_length);
	tvb_set_child_real_data_tvbuff(tvb, extracted_tvb);
	add_new_data_source(pinfo, extracted_tvb, "P25 CAI");

	return extracted_tvb;
}

/* Build Header Data Unit tvb. */
tvbuff_t*
build_hdu_tvb(tvbuff_t *tvb, packet_info *pinfo, int offset)
{
	guint8 *hdu_buffer, *rs_codeword;
	guint8 rs_code_byte, high_byte, low_byte;
	guint32 golay_codeword;
	int i, j;
	tvbuff_t *hdu_tvb;

	DISSECTOR_ASSERT(tvb_length_remaining(tvb, offset) >= 81);

	rs_codeword = (guint8*)ep_alloc0(27);
	hdu_buffer = (guint8*)ep_alloc0(15);

	/* Each 18 bits is a Golay codeword. */
	for (i = offset * 8, j = 0; j < 216; i += 18, j += 6) {

		/* Take 18 bits from the tvb, adjusting for byte boundaries. */
		golay_codeword = (tvb_get_ntohl(tvb, i / 8) >> (14 - i % 8)) & 0x3FFFF;
		rs_code_byte = golay_18_6_8_decode(golay_codeword) << 2;

		/* Stuff high bits into one byte of the new buffer. */
		high_byte = rs_code_byte >> (j % 8);
		rs_codeword[j / 8] |= high_byte;

		/* Stuff low bits into the next unless beyond end of buffer. */
		if (j < 210) {
			low_byte = rs_code_byte << (8 - j % 8);
			rs_codeword[j / 8 + 1] |= low_byte;
		}
	}

	rs_36_20_17_decode(rs_codeword, hdu_buffer);

	/* Setup a new tvb buffer with the decoded data. */
	hdu_tvb = tvb_new_real_data(hdu_buffer, 15, 15);
	tvb_set_child_real_data_tvbuff(tvb, hdu_tvb);
	add_new_data_source(pinfo, hdu_tvb, "data units");

	return hdu_tvb;
}

/* Build Low Speed Data tvb from LDU */
tvbuff_t*
build_ldu_lsd_tvb(tvbuff_t *tvb, packet_info *pinfo, int offset)
{
	guint8 *lsd_buffer;
	tvbuff_t *lsd_tvb;

	/* we were passed the offset to the beginning of the LDU */
	offset += 174;

	DISSECTOR_ASSERT(tvb_length_remaining(tvb, offset) >= 4);

	lsd_buffer = (guint8*)ep_alloc0(2);

	cyclic_16_8_5_decode(tvb_get_ntohl(tvb, offset), lsd_buffer);

	/* Setup a new tvb buffer with the decoded data. */
	lsd_tvb = tvb_new_real_data(lsd_buffer, 2, 2);
	tvb_set_child_real_data_tvbuff(tvb, lsd_tvb);
	add_new_data_source(pinfo, lsd_tvb, "Low Speed Data");

	return lsd_tvb;
}

/* Build Link Control tvb from LDU1 */
tvbuff_t*
build_ldu_lc_tvb(tvbuff_t *tvb, packet_info *pinfo, int offset)
{
	guint8 *lc_buffer, *rs_codeword;
	guint8 rs_code_byte, high_byte, low_byte;
	guint32 hamming_codeword;
	int i, j, r, t;
	tvbuff_t *lc_tvb;

	/* we were passed the offset to the beginning of the LDU */
	offset += 36;

	DISSECTOR_ASSERT(tvb_length_remaining(tvb, offset) >= 147);

	rs_codeword = (guint8*)ep_alloc0(18);
	lc_buffer = (guint8*)ep_alloc0(9);

	/* step through tvb bits to find 10 bit hamming codewords */
	for (i = offset * 8, r = 0; r < 144; i += 184) {
		for (j = 0; j < 40; j += 10, r += 6) {
			/* t = tvb bit index
			 * r = reed-solomon codeword bit index
			 */
			t = i + j;
			hamming_codeword = (tvb_get_ntohl(tvb, t / 8) >> (22 - t % 8)) & 0x3FF;
			rs_code_byte = hamming_10_6_3_decode(hamming_codeword) << 2;

			/* Stuff high bits into one byte of the new buffer. */
			high_byte = rs_code_byte >> (r % 8);
			rs_codeword[r / 8] |= high_byte;

			/* Stuff low bits into the next unless beyond end of buffer. */
			if (r < 144) {
				low_byte = rs_code_byte << (8 - r % 8);
				rs_codeword[r / 8 + 1] |= low_byte;
			}
		}
	}

	rs_24_12_13_decode(rs_codeword, lc_buffer);

	/* Setup a new tvb buffer with the decoded data. */
	lc_tvb = tvb_new_real_data(lc_buffer, 9, 9);
	tvb_set_child_real_data_tvbuff(tvb, lc_tvb);
	add_new_data_source(pinfo, lc_tvb, "Link Control");

	return lc_tvb;
}

/* Build Encryption Sync tvb LDU2 */
tvbuff_t*
build_ldu_es_tvb(tvbuff_t *tvb, packet_info *pinfo, int offset)
{
	guint8 *es_buffer, *rs_codeword;
	guint8 rs_code_byte, high_byte, low_byte;
	guint32 hamming_codeword;
	int i, j, r, t;
	tvbuff_t *es_tvb;

	/* we were passed the offset to the beginning of the LDU */
	offset += 36;

	DISSECTOR_ASSERT(tvb_length_remaining(tvb, offset) >= 147);

	rs_codeword = (guint8*)ep_alloc0(18);
	es_buffer = (guint8*)ep_alloc0(12);

	/* step through tvb bits to find 10 bit hamming codewords */
	for (i = offset * 8, r = 0; r < 144; i += 184) {
		for (j = 0; j < 40; j += 10, r += 6) {
			/* t = tvb bit index
			 * r = reed-solomon codeword bit index
			 */
			t = i + j;
			hamming_codeword = (tvb_get_ntohl(tvb, t / 8) >> (22 - t % 8)) & 0x3FF;
			rs_code_byte = hamming_10_6_3_decode(hamming_codeword) << 2;

			/* Stuff high bits into one byte of the new buffer. */
			high_byte = rs_code_byte >> (r % 8);
			rs_codeword[r / 8] |= high_byte;

			/* Stuff low bits into the next unless beyond end of buffer. */
			if (r < 144) {
				low_byte = rs_code_byte << (8 - r % 8);
				rs_codeword[r / 8 + 1] |= low_byte;
			}
		}
	}

	rs_24_16_9_decode(rs_codeword, es_buffer);

	/* Setup a new tvb buffer with the decoded data. */
	es_tvb = tvb_new_real_data(es_buffer, 12, 12);
	tvb_set_child_real_data_tvbuff(tvb, es_tvb);
	add_new_data_source(pinfo, es_tvb, "Encryption Sync");

	return es_tvb;
}

/* Build Trunking Signaling Block tvb. */
tvbuff_t*
build_tsdu_tvb(tvbuff_t *tvb, packet_info *pinfo, int offset)
{
	guint8 *tsdu_buffer, *trellis_buffer;
	guint8 last_block;
	int tsdu_offset, tvb_bit_offset;
	tvbuff_t *tsdu_tvb;

	/* From here on, our tvb offset may not fall on bit boundaries, so we track
	 * it by bits instead of bytes.
	 */
	tvb_bit_offset = offset * 8;
	tsdu_offset = 0;
	last_block = 0;

	tsdu_buffer = (guint8*)ep_alloc0(36); /* 12 times maximum number of TSBKs (3) */

	while (last_block == 0) {
		DISSECTOR_ASSERT(tvb_length_remaining(tvb, tvb_bit_offset / 8) >= 25);
		trellis_buffer = (guint8*)ep_alloc0(25);
		data_deinterleave(tvb, trellis_buffer, tvb_bit_offset);
		trellis_1_2_decode(trellis_buffer, tsdu_buffer, tsdu_offset);
		tvb_bit_offset += 196;
		last_block = tsdu_buffer[tsdu_offset] >> 7;
		tsdu_offset += 12;
	}

	/* Setup a new tvb buffer with the decoded data. */
	tsdu_tvb = tvb_new_real_data(tsdu_buffer, tsdu_offset, tsdu_offset);
	tvb_set_child_real_data_tvbuff(tvb, tsdu_tvb);
	add_new_data_source(pinfo, tsdu_tvb, "data units");

	return tsdu_tvb;
}

/* Build Link Control tvb from Terminator */
tvbuff_t*
build_term_lc_tvb(tvbuff_t *tvb, packet_info *pinfo, int offset)
{
	guint8 *lc_buffer, *rs_codeword;
	guint16 rs_code_chunk;
	guint8 high_byte, low_byte;
	guint32 golay_codeword;
	int i, j;
	tvbuff_t *lc_tvb;

	DISSECTOR_ASSERT(tvb_length_remaining(tvb, offset) >= 36);

	rs_codeword = (guint8*)ep_alloc0(18);
	lc_buffer = (guint8*)ep_alloc0(9);

	/* Each 24 bits is a Golay codeword. */
	for (i = offset * 8, j = 0; j < 144; i += 24, j += 12) {

		/* Take 24 bits from the tvb, adjusting for byte boundaries. */
		golay_codeword = (tvb_get_ntohl(tvb, i / 8) >> (8 - i % 8)) & 0xFFFFFF;
		rs_code_chunk = golay_24_12_8_decode(golay_codeword) << 4;

		/* Stuff high bits into one byte of the new buffer. */
		high_byte = rs_code_chunk >> (j % 8);
		rs_codeword[j / 8] |= high_byte;

		/* Stuff low bits into the next. */
		low_byte = rs_code_chunk << (8 - j % 8);
		rs_codeword[j / 8 + 1] |= low_byte;
	}

	rs_24_12_13_decode(rs_codeword, lc_buffer);

	/* Setup a new tvb buffer with the decoded data. */
	lc_tvb = tvb_new_real_data(lc_buffer, 9, 9);
	tvb_set_child_real_data_tvbuff(tvb, lc_tvb);
	add_new_data_source(pinfo, lc_tvb, "data units");

	return lc_tvb;
}

/* Deinterleave data block.  Assumes output buffer is already zeroed. */
void
data_deinterleave(tvbuff_t *tvb, guint8 *deinterleaved, int bit_offset)
{
	int d, i, j, t;
	int steps[] = {0, 52, 100, 148};

	/* step through input nibbles to copy to output */
	for (i = bit_offset, d = 0; i < 45 + bit_offset; i += 4) {
		for (j = 0; j < 4; j++, d += 4) {
			/* t = tvb bit index
			 * d = deinterleaved bit index
			 */
			t = i + steps[j];
			deinterleaved[d / 8] |= ((tvb_get_guint8(tvb, t / 8) << (t % 8)) & 0xF0) >> (d % 8);
		}
	}
	t = bit_offset + 48;
	deinterleaved[24] |= ((tvb_get_guint8(tvb, t / 8) << (t % 8)) & 0xF0);
}

/* 1/2 rate trellis decoder.  Assumes output buffer is already zeroed. */
void
trellis_1_2_decode(guint8 *encoded, guint8 *decoded, int offset)
{
	int i, j;
	int state = 0;
	guint8 codeword;

	/* Hamming distance */
	guint8 hd[4];

	/* state transition table, including constellation to dibit pair mapping */
	guint8 next_words[4][4] = {
		{0x2, 0xC, 0x1, 0xF},
		{0xE, 0x0, 0xD, 0x3},
		{0x9, 0x7, 0xA, 0x4},
		{0x5, 0xB, 0x6, 0x8}
	};

	/* step through 4 bit codewords in input */
	for (i = 0; i < 196; i += 4) {
		codeword = (encoded[i / 8] >> (4 - (i % 8))) & 0xF;
		/* try each codeword in a row of the state transition table */
		for (j = 0; j < 4; j++) {
			/* find Hamming distance for candidate */
			hd[j] = count_bits(codeword ^ next_words[state][j]);
		}
		/* find the dibit that matches the most codeword bits (minimum Hamming distance) */
		state = find_min(hd, 4);
		/* error if minimum can't be found */
		DISSECTOR_ASSERT(state != -1);
		/* It also might be nice to report a condition where the minimum is
		 * non-zero, i.e. an error has been corrected.  It probably shouldn't
		 * be a permanent failure, though.
		 *
		 * DISSECTOR_ASSERT(hd[state] == 0);
		 */

		/* append dibit onto output buffer */
		if (i < 192)
			decoded[(i / 16) + offset] |= state << (6 - (i / 2) % 8);
	}
}

/* 3/4 rate trellis decoder.  Assumes output buffer is already zeroed. */
void
trellis_3_4_decode(guint8 *encoded, guint8 *decoded, int offset)
{
	int i, j;
	int state = 0;
	guint8 codeword;

	/* Hamming distance */
	guint8 hd[8];

	/* state transition table, including constellation to dibit pair mapping */
	guint8 next_words[8][8] = {
		{0x2, 0xD, 0xE, 0x1, 0x7, 0x8, 0xB, 0x4},
		{0xE, 0x1, 0x7, 0x8, 0xB, 0x4, 0x2, 0xD},
		{0xA, 0x5, 0x6, 0x9, 0xF, 0x0, 0x3, 0xC},
		{0x6, 0x9, 0xF, 0x0, 0x3, 0xC, 0xA, 0x5},
		{0xF, 0x0, 0x3, 0xC, 0xA, 0x5, 0x6, 0x9},
		{0x3, 0xC, 0xA, 0x5, 0x6, 0x9, 0xF, 0x0},
		{0x7, 0x8, 0xB, 0x4, 0x2, 0xD, 0xE, 0x1},
		{0xB, 0x4, 0x2, 0xD, 0xE, 0x1, 0x7, 0x8}
	};

	/* step through 4 bit codewords in input */
	for (i = 0; i < 196; i += 4) {
		codeword = (encoded[i / 8] >> (4 - (i % 8))) & 0xF;
		/* try each codeword in a row of the state transition table */
		for (j = 0; j < 8; j++) {
			/* find Hamming distance for candidate */
			hd[j] = count_bits(codeword ^ next_words[state][j]);
		}
		/* find the dibit that matches the most codeword bits (minimum Hamming distance) */
		state = find_min(hd, 8);
		/* error if minimum can't be found */
		DISSECTOR_ASSERT(state != -1);
		/* It also might be nice to report a condition where the minimum is
		 * non-zero, i.e. an error has been corrected.  It probably shouldn't
		 * be a permanent failure, though.
		 *
		 * DISSECTOR_ASSERT(hd[state] == 0);
		 */

		/* append tribit onto output buffer */
		if (i < 192)
			/* FIXME adapt from 1/2 to 3/4 rate */
			decoded[(i / 16) + offset] |= state << (6 - (i / 2) % 8);
	}
}

/* Error correction decoders
 *
 * TODO:  For now these are fake.  They pull out the original bits without
 * actually doing any error correction or detection.
 */

/* fake (18,6,8) shortened Golay decoder, no error correction */
/* TODO: make less fake */
guint8
golay_18_6_8_decode(guint32 codeword)
{
	return (codeword >> 12) & 0x3F;
}

/* fake (24,12,8) extended Golay decoder, no error correction */
/* TODO: make less fake */
guint16
golay_24_12_8_decode(guint32 codeword)
{
	return (codeword >> 12) & 0xFFF;
}

/* fake (10,6,3) shortened Hamming decoder, no error correction */
/* TODO: make less fake */
guint8
hamming_10_6_3_decode(guint32 codeword)
{
	return (codeword >> 4) & 0x3F;
}

/* fake (16,8,5) shortened cyclic decoder, no error correction */
/* TODO: make less fake */
void
cyclic_16_8_5_decode(guint32 codeword, guint8 *decoded)
{
	/* Take the first byte of each 16 bit word. */
	decoded[0] = codeword >> 24;
	decoded[1] = codeword >> 8;
}

/* fake (24,12,13) Reed-Solomon decoder, no error correction */
/* TODO: make less fake */
void
rs_24_12_13_decode(guint8 *codeword, guint8 *decoded)
{
	int i;

	/* Just grab the first 9 bytes (12 sets of six bits) */
	for (i = 0; i < 9; i++)
		decoded[i] = codeword[i];
}

/* fake (24,16,9) Reed-Solomon decoder, no error correction */
/* TODO: make less fake */
void
rs_24_16_9_decode(guint8 *codeword, guint8 *decoded)
{
	int i;

	/* Just grab the first 12 bytes (16 sets of six bits) */
	for (i = 0; i < 12; i++)
		decoded[i] = codeword[i];
}

/* fake (36,20,17) Reed-Solomon decoder, no error correction */
/* TODO: make less fake */
void
rs_36_20_17_decode(guint8 *codeword, guint8 *decoded)
{
	int i;

	/* Just grab the first 15 bytes (20 sets of six bits) */
	for (i = 0; i < 15; i++)
		decoded[i] = codeword[i];
}

/* Register the protocol with Wireshark */
void
proto_register_p25cai(void)
{

	/* Setup list of fields */
	static hf_register_info hf[] = {
		{ &hf_p25cai_fs,
			{ "Frame Synchronization", "p25cai.fs",
			FT_BYTES, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_nid,
			{ "Network ID Codeword", "p25cai.nid",
			FT_UINT64, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_nac,
			{ "Network Access Code", "p25cai.nac",
			FT_UINT16, BASE_HEX, VALS(network_access_codes), 0xFFF0,
			NULL, HFILL }
		},
		{ &hf_p25cai_duid,
			{ "Data Unit ID", "p25cai.duid",
			FT_UINT16, BASE_HEX, VALS(data_unit_ids), 0x000F,
			NULL, HFILL }
		},
		{ &hf_p25cai_hdu,
			{ "Header Data Unit", "p25cai.hdu",
			FT_NONE, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_tsbk,
			{ "Trunking Signaling Block", "p25cai.tsbk",
			FT_NONE, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_pdu,
			{ "Packet Data Unit", "p25cai.pdu",
			FT_NONE, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_ldu1,
			{ "Logical Link Data Unit 1", "p25cai.ldu1",
			FT_NONE, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_ldu2,
			{ "Logical Link Data Unit 2", "p25cai.ldu2",
			FT_NONE, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_mi,
			{ "Message Indicator", "p25cai.mi",
			FT_BYTES, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_mfid,
			{ "Manufacturer's ID", "p25cai.mfid",
			FT_UINT8, BASE_HEX, VALS(manufacturer_ids), 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_algid,
			{ "Algorithm ID", "p25cai.algid",
			FT_UINT8, BASE_HEX, VALS(algorithm_ids), 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_kid,
			{ "Key ID", "p25cai.kid",
			FT_UINT16, BASE_HEX, VALS(key_ids), 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_tgid,
			{ "Talk Group ID", "p25cai.tgid",
			FT_UINT16, BASE_HEX, VALS(talk_group_ids), 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_ss_parent,
			{ "Status Symbols", "p25cai.ss_parent",
			FT_NONE, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_ss,
			{ "Status Symbol", "p25cai.ss",
			FT_UINT8, BASE_HEX, VALS(status_symbols), 0x3,
			NULL, HFILL }
		},
		{ &hf_p25cai_lc,
			{ "Link Control", "p25cai.lc",
			FT_NONE, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_lcf,
			{ "Link Control Format", "p25cai.lcf",
			FT_UINT8, BASE_HEX, VALS(link_control_formats), 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_lbf,
			{ "Last Block Flag", "p25cai.lbf",
			FT_BOOLEAN, BASE_NONE, NULL, 0x80,
			NULL, HFILL }
		},
		{ &hf_p25cai_ptbf,
			{ "Protected Trunking Block Flag", "p25cai.ptbf",
			FT_BOOLEAN, BASE_NONE, NULL, 0x40,
			NULL, HFILL }
		},
		{ &hf_p25cai_isp_opcode,
			{ "Opcode", "p25cai.isp.opcode",
			FT_UINT8, BASE_HEX, VALS(isp_opcodes), 0x3F,
			NULL, HFILL }
		},
		{ &hf_p25cai_osp_opcode,
			{ "Opcode", "p25cai.osp.opcode",
			FT_UINT8, BASE_HEX, VALS(osp_opcodes), 0x3F,
			NULL, HFILL }
		},
		{ &hf_p25cai_unknown_opcode,
			{ "Unknown Opcode (non-standard MFID)", "p25cai.unknown.opcode",
			FT_UINT8, BASE_HEX, NULL, 0x3F,
			NULL, HFILL }
		},
		{ &hf_p25cai_args,
			{ "Arguments", "p25cai.args",
			FT_UINT64, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_crc,
			{ "CRC", "p25cai.crc",
			FT_UINT16, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_imbe,
			{ "Raw IMBE Frame", "p25cai.imbe",
			FT_BYTES, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_lsd,
			{ "Low Speed Data", "p25cai.lsd",
			FT_UINT16, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_es,
			{ "Encryption Sync", "p25cai.es",
			FT_NONE, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_an,
			{ "A/N", "p25cai.an",
			FT_BOOLEAN, BASE_NONE, NULL, 0x40,
			NULL, HFILL }
		},
		{ &hf_p25cai_io,
			{ "I/O", "p25cai.io",
			FT_BOOLEAN, BASE_NONE, TFS(&inbound_outbound), 0x20,
			NULL, HFILL }
		},
		{ &hf_p25cai_pdu_format,
			{ "Format", "p25cai.pdu.format",
			FT_UINT8, BASE_HEX, VALS(pdu_formats), 0x1F,
			NULL, HFILL }
		},
		{ &hf_p25cai_sapid,
			{ "SAP ID", "p25cai.sapid",
			FT_UINT8, BASE_HEX, VALS(service_access_points), 0x3F,
			NULL, HFILL }
		},
		{ &hf_p25cai_llid,
			{ "Logical Link ID", "p25cai.llid",
			FT_UINT24, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_fmf,
			{ "Full Message Flag", "p25cai.fmf",
			FT_BOOLEAN, BASE_NONE, NULL, 0x80,
			NULL, HFILL }
		},
		{ &hf_p25cai_btf,
			{ "Blocks to Follow", "p25cai.btf",
			FT_UINT8, BASE_DEC, NULL, 0x7F,
			NULL, HFILL }
		},
		{ &hf_p25cai_poc,
			{ "Pad Octet Count", "p25cai.poc",
			FT_UINT8, BASE_HEX, NULL, 0x1F,
			NULL, HFILL }
		},
		{ &hf_p25cai_syn,
			{ "Syn", "p25cai.syn",
			FT_BOOLEAN, BASE_NONE, NULL, 0x80,
			NULL, HFILL }
		},
		{ &hf_p25cai_ns,
			{ "N(S)", "p25cai.ns",
			FT_UINT8, BASE_HEX, NULL, 0x70,
			NULL, HFILL }
		},
		{ &hf_p25cai_fsnf,
			{ "Fragment Sequence Number Field", "p25cai.fsnf",
			FT_UINT8, BASE_HEX, NULL, 0x0F,
			NULL, HFILL }
		},
		{ &hf_p25cai_dho,
			{ "Data Header Offset", "p25cai.dho",
			FT_UINT8, BASE_HEX, NULL, 0x3F,
			NULL, HFILL }
		},
		{ &hf_p25cai_db,
			{ "Data Block", "p25cai.db",
			FT_NONE, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_dbsn,
			{ "Data Block Serial Number", "p25cai.dbsn",
			FT_UINT8, BASE_HEX, NULL, 0xFE,
			NULL, HFILL }
		},
		{ &hf_p25cai_crc9,
			{ "CRC", "p25cai.crc9",
			FT_UINT16, BASE_HEX, NULL, 0x1FF,
			NULL, HFILL }
		},
		{ &hf_p25cai_ud,
			{ "User Data", "p25cai.ud",
			FT_BYTES, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_packet_crc,
			{ "Packet CRC", "p25cai.packet_crc",
			FT_UINT32, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_class,
			{ "Response Class", "p25cai.class",
			FT_UINT8, BASE_HEX, NULL, 0xA0,
			NULL, HFILL }
		},
		{ &hf_p25cai_type,
			{ "Response Type", "p25cai.type",
			FT_UINT8, BASE_HEX, NULL, 0x38,
			NULL, HFILL }
		},
		{ &hf_p25cai_status,
			{ "Response Status", "p25cai.status",
			FT_UINT8, BASE_HEX, NULL, 0x07,
			NULL, HFILL }
		},
		{ &hf_p25cai_x,
			{ "X", "p25cai.x",
			FT_BOOLEAN, BASE_HEX, NULL, 0x80,
			NULL, HFILL }
		},
		{ &hf_p25cai_sllid,
			{ "Source Logical Link ID", "p25cai.sllid",
			FT_UINT24, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		}
	};

	/* Setup protocol subtree arrays */
	static gint *ett[] = {
		&ett_p25cai,
		&ett_ss,
		&ett_nid,
		&ett_du,
		&ett_lc,
		&ett_es,
		&ett_db
	};

	/* Register the protocol name and description */
	proto_p25cai = proto_register_protocol(
		"APCO Project 25 Common Air Interface",		/* full name */
		"P25 CAI",	/* short name */
		"p25cai"	/* abbreviation (e.g. for filters) */
		);

	/* Required function calls to register the header fields and subtrees used */
	proto_register_field_array(proto_p25cai, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

}

void
proto_reg_handoff_p25cai(void)
{
	static gboolean inited = FALSE;

	if (!inited) {

		dissector_handle_t p25cai_handle;

		/*  Use new_create_dissector_handle() to indicate that dissect_p25cai()
		 *  returns the number of bytes it dissected (or 0 if it thinks the packet
		 *  does not belong to APCO Project 25 Common Air Interface).
		 */
		p25cai_handle = new_create_dissector_handle(dissect_p25cai, proto_p25cai);

		/* FIXME: Temporarily using an unassigned UDP port! Application
		 * for an IANA-assigned port number has been made.
		 */
		dissector_add("udp.port", 23456, p25cai_handle);

		inited = TRUE;
	}
}


/* Utility functions */

/* count the number of 1 bits in an int */
int
count_bits(unsigned int n)
{
    int i = 0;
    for (i = 0; n != 0; i++)
        n &= n - 1;
    return i;
}

/* return the index of the lowest value in a list */
int
find_min(guint8 list[], int len)
{
	int min = list[0];	
	int index = 0;	
	int unique = 1;	
	int i;

	for (i = 1; i < len; i++) {
		if (list[i] < min) {
			min = list[i];
			index = i;
			unique = 1;
		} else if (list[i] == min) {
			unique = 0;
		}
	}
	/* return -1 if a minimum can't be found */
	if (!unique)
		return -1;

	return index;
}
