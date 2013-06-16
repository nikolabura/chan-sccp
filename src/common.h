/*!
 * \file        common.h
 * \brief       SCCP Common Include File
 * \author      Diederik de Groot <dkgroot [at] talon.nl>
 * \note        This file is automatically generated by configure
 * \note        Do not change this file. Change will be lost when running configure
 */

#ifndef CHAN_SCCP_COMMON_H
#define CHAN_SCCP_COMMON_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if HAVE_SYS_SIGNAL_H
#include <sys/signal.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_CTYPE_H
#include <ctype.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#if HAVE_BYTESWAP_H
#include <byteswap.h>
#elif HAVE_SYS_BYTEORDER_H
#include <sys/byteorder.h>
#elif HAVE_SYS_ENDIAN_H
#include <sys/endian.h>
#endif
#ifdef HAVE_ATOMIC_OPS_H
#include <atomic_ops.h>
#endif

#include "sccp_lock.h"
#include "sccp_refcount.h"
#include "sccp_dllists.h"
#include "sccp_threadpool.h"
#include "chan_sccp.h"
#include "sccp_event.h"
#include "pbx_impl/pbx_impl.h"

#include "sccp_pbx.h"
#include "sccp_protocol.h"
#include "sccp_socket.h"
#include "sccp_device.h"
#include "sccp_line.h"
#include "sccp_channel.h"
#include "sccp_features.h"
#include "sccp_utils.h"
#include "sccp_indicate.h"
#include "sccp_hint.h"
#include "sccp_actions.h"
#include "sccp_featureButton.h"
#include "sccp_mwi.h"
#include "sccp_config.h"
#include "sccp_conference.h"
#include "sccp_labels.h"
#include "sccp_softkeys.h"
#include "sccp_conference.h"
#include "sccp_features.h"
#include "sccp_adv_features.h"
#include "sccp_cli.h"
#include "sccp_appfunctions.h"
#include "sccp_management.h"
#include "sccp_rtp.h"

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif														// CHAN_SCCP_COMMON_H
