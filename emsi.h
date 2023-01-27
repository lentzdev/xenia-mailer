/* Xenia Mailer - FidoNet Technology Mailer
 * Copyright (C) 1987-2001 Arjen G. Lentz
 *
 * This file is part of Xenia Mailer.
 * Xenia Mailer is free software; you can redistribute it and/or
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#ifndef H_EMSI
#define H_EMSI


#define EMSI_INQ  0
#define EMSI_REQ  1
#define EMSI_CLI  2     /* IEMSI Client */
#define EMSI_HBT  3
#define EMSI_DAT  4
#define EMSI_ACK  5
#define EMSI_NAK  6
#define EMSI_MD5  7
#define EMSI_M5R  8
#define EMSI_IRQ  9     /* IEMSI identifies server as IEMSI capable     */
#define EMSI_IIR 10     /* IEMSI Interactive Interrupt Request (abort)  */
#define EMSI_ICI 11     /* IEMSI Client Information                     */
#define EMSI_ISI 12     /* IEMSI Server Information                     */
#define EMSI_CHT 13     /* IEMSI Initiate Full Screen Chat              */
#define EMSI_TCH 14     /* IEMSI Terminate Full Screen Chat             */

#define EMSI_IDLEN     14
#define MAX_DATLEN   8192               /* Maximum size of EMSI_DAT packet */

/* Return values from get_emsi_dat() */
#define EMSI_SUCCESS    0
#define EMSI_ABORT     -1
#define EMSI_RETRY     -2
#define EMSI_BADHEX    -3
#define EMSI_BADLEN    -4
#define EMSI_BADCRC    -5
#define EMSI_TIMEOUT   -6
#define EMSI_CARRIER   -7
#define EMSI_SYSABORT  -8

/* Prototypes */
int     check_emsi   (char c, char *buf);    /* check incoming data for EMSI */
void    put_emsi     (int type, boolean cr); /* transmit an EMSI packet      */
boolean recv_emsi    (boolean send_too);     /* EMSI handshake; receiver     */
boolean send_emsi    (void);                 /* EMSI handshake; originator   */
void    client_iemsi (char *user_password, byte emu);   /* IEMSI client      */
void    emsi_md5make (void);
int     emsi_md5get  (char *emsibuf);

#endif  /* H_EMSI */


/* end of emsi.h */
