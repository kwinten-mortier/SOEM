/*
 * Licensed under the GNU General Public License version 2 with exceptions. See
 * LICENSE file in the project root for full license information
 */

/** \file
 * \brief
 * Base EtherCAT functions.
 *
 * Setting up a datagram in an ethernet frame.
 * EtherCAT datagram primitives, broadcast, auto increment, configured and
 * logical addressed data transfers. All base transfers are blocking, so
 * wait for the frame to be returned to the master or timeout. If this is
 * not acceptable build your own datagrams and use the functions from nicdrv.c.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "oshw.h"
#include "osal.h"
#include "ethercattype.h"
#include "ethercatbase.h"

/** Write data to EtherCAT datagram.
 *
 * @param[out] datagramdata   = data part of datagram
 * @param[in]  com            = command
 * @param[in]  length         = length of databuffer
 * @param[in]  data           = databuffer to be copied into datagram
 */
static void ecx_writedatagramdata(void *datagramdata, ec_cmdtype com, uint16 length, const void * data)
{
   if (length > 0)
   {
      switch (com)
      {
         // case EC_CMD_NOP:
            /* Fall-through */
         case EC_CMD_APRD:
            /* Fall-through */
         case EC_CMD_FPRD:
            /* Fall-through */
         case EC_CMD_BRD:
            /* Fall-through */
         case EC_CMD_LRD:
            /* no data to write. initialise data so frame is in a known state */
            memset(datagramdata, 0, length);
            break;
         default:
            memcpy(datagramdata, data, length);
            break;
      }
   }
}

/** Generate and set EtherCAT datagram in a standard ethernet frame.
 *
 * @param[in] port        = port context struct
 * @param[out] frame       = framebuffer
 * @param[in]  com         = command
 * @param[in]  idx         = index used for TX and RX buffers
 * @param[in]  ADP         = Address Position
 * @param[in]  ADO         = Address Offset
 * @param[in]  length      = length of datagram excluding EtherCAT header
 * @param[in]  data        = databuffer to be copied in datagram
 * @return always 0
 */
int ecx_setupdatagram(ecx_portt *port, void *frame, uint8 com, uint8 idx, uint16 ADP, uint16 ADO, uint16 length, void *data)
{
   ec_comt *datagramP;
   uint8 *frameP;

   frameP = frame;
   /* Ethernet header is preset and fixed in frame buffers
      EtherCAT header needs to be added after that */
   datagramP = (ec_comt*)&frameP[ETH_HEADERSIZE];
   datagramP->elength = htoes(EC_ECATTYPE + EC_HEADERSIZE + length);
   datagramP->command = com;
   datagramP->index = idx;
   datagramP->ADP = htoes(ADP);
   datagramP->ADO = htoes(ADO);
   datagramP->dlength = htoes(length);
   ecx_writedatagramdata(&frameP[ETH_HEADERSIZE + EC_HEADERSIZE], com, length, data);
   /* set WKC to zero */
   frameP[ETH_HEADERSIZE + EC_HEADERSIZE + length] = 0x00;
   frameP[ETH_HEADERSIZE + EC_HEADERSIZE + length + 1] = 0x00;
   /* set size of frame in buffer array */
   port->txbuflength[idx] = ETH_HEADERSIZE + EC_HEADERSIZE + EC_WKCSIZE + length;

   return 0;
}

/** Add EtherCAT datagram to a standard ethernet frame with existing datagram(s).
 *
 * @param[in] port        = port context struct
 * @param[out] frame      = framebuffer
 * @param[in]  com        = command
 * @param[in]  idx        = index used for TX and RX buffers
 * @param[in]  more       = TRUE if still more datagrams to follow
 * @param[in]  ADP        = Address Position
 * @param[in]  ADO        = Address Offset
 * @param[in]  length     = length of datagram excluding EtherCAT header
 * @param[in]  data       = databuffer to be copied in datagram
 * @return Offset to data in rx frame, usefull to retrieve data after RX.
 */
uint16 ecx_adddatagram(ecx_portt *port, void *frame, uint8 com, uint8 idx, boolean more, uint16 ADP, uint16 ADO, uint16 length, void *data)
{
   ec_comt *datagramP;
   uint8 *frameP;
   uint16 prevlength;

   frameP = frame;
   /* copy previous frame size */
   prevlength = (uint16)port->txbuflength[idx];
   datagramP = (ec_comt*)&frameP[ETH_HEADERSIZE];
   /* add new datagram to ethernet frame size */
   datagramP->elength = htoes( etohs(datagramP->elength) + EC_HEADERSIZE + length );
   /* add "datagram follows" flag to previous subframe dlength */
   datagramP->dlength = htoes( etohs(datagramP->dlength) | EC_DATAGRAMFOLLOWS );
   /* set new EtherCAT header position */
   datagramP = (ec_comt*)&frameP[prevlength - EC_ELENGTHSIZE];
   datagramP->command = com;
   datagramP->index = idx;
   datagramP->ADP = htoes(ADP);
   datagramP->ADO = htoes(ADO);
   if (more)
   {
      /* this is not the last datagram to add */
      datagramP->dlength = htoes(length | EC_DATAGRAMFOLLOWS);
   }
   else
   {
      /* this is the last datagram in the frame */
      datagramP->dlength = htoes(length);
   }
   ecx_writedatagramdata(&frameP[prevlength + EC_HEADERSIZE - EC_ELENGTHSIZE], com, length, data);
   /* set WKC to zero */
   frameP[prevlength + EC_HEADERSIZE - EC_ELENGTHSIZE + length] = 0x00;
   frameP[prevlength + EC_HEADERSIZE - EC_ELENGTHSIZE + length + 1] = 0x00;
   /* set size of frame in buffer array */
   port->txbuflength[idx] = prevlength + EC_HEADERSIZE - EC_ELENGTHSIZE + EC_WKCSIZE + length;

   /* return offset to data in rx frame
      14 bytes smaller than tx frame due to stripping of ethernet header */
   return prevlength + EC_HEADERSIZE - EC_ELENGTHSIZE - ETH_HEADERSIZE;
}

/** BRW "broadcast write" primitive. Blocking.
 *
 * @param[in] port        = port context struct
 * @param[in] ADP         = Address Position, normally 0
 * @param[in] ADO         = Address Offset, slave memory address
 * @param[in] length      = length of databuffer
 * @param[in] data        = databuffer to be written to slaves
 * @param[in] timeout     = timeout in us, standard is EC_TIMEOUTRET
 * @return Workcounter or EC_NOFRAME
 */
int ecx_BWR (ecx_portt *port, uint16 ADP, uint16 ADO, uint16 length, void *data, int timeout)
{
   uint8 idx;
   int wkc;

   /* get fresh index */
   idx = ecx_getindex (port);
   /* setup datagram */
   ecx_setupdatagram (port, &(port->txbuf[idx]), EC_CMD_BWR, idx, ADP, ADO, length, data);
   /* send data and wait for answer */
   wkc = ecx_srconfirm (port, idx, timeout);
   /* clear buffer status */
   ecx_setbufstat (port, idx, EC_BUF_EMPTY);

   return wkc;
}

/** BRD "broadcast read" primitive. Blocking.
 *
 * @param[in] port        = port context struct
 * @param[in]  ADP        = Address Position, normally 0
 * @param[in]  ADO        = Address Offset, slave memory address
 * @param[in]  length     = length of databuffer
 * @param[out] data       = databuffer to put slave data in
 * @param[in]  timeout    = timeout in us, standard is EC_TIMEOUTRET
 * @return Workcounter or EC_NOFRAME
 */
int ecx_BRD(ecx_portt *port, uint16 ADP, uint16 ADO, uint16 length, void *data, int timeout)
{
   uint8 idx;
   int wkc;

   /* get fresh index */
   idx = ecx_getindex(port);
   /* setup datagram */
   ecx_setupdatagram(port, &(port->txbuf[idx]), EC_CMD_BRD, idx, ADP, ADO, length, data);
   /* send data and wait for answer */
   wkc = ecx_srconfirm (port, idx, timeout);
   if (wkc > 0)
   {
      /* copy datagram to data buffer */
      memcpy(data, &(port->rxbuf[idx][EC_HEADERSIZE]), length);
   }
   /* clear buffer status */
   ecx_setbufstat(port, idx, EC_BUF_EMPTY);

   return wkc;
}

/** APRD "auto increment address read" primitive. Blocking.
 *
 * @param[in] port        = port context struct
 * @param[in]  ADP        = Address Position, each slave ++, slave that has 0 executes
 * @param[in]  ADO        = Address Offset, slave memory address
 * @param[in]  length     = length of databuffer
 * @param[out] data       = databuffer to put slave data in
 * @param[in]  timeout    = timeout in us, standard is EC_TIMEOUTRET
 * @return Workcounter or EC_NOFRAME
 */
int ecx_APRD(ecx_portt *port, uint16 ADP, uint16 ADO, uint16 length, void *data, int timeout)
{
   int wkc;
   uint8 idx;

   idx = ecx_getindex(port);
   ecx_setupdatagram(port, &(port->txbuf[idx]), EC_CMD_APRD, idx, ADP, ADO, length, data);
   wkc = ecx_srconfirm(port, idx, timeout);
   if (wkc > 0)
   {
      memcpy(data, &(port->rxbuf[idx][EC_HEADERSIZE]), length);
   }
   ecx_setbufstat(port, idx, EC_BUF_EMPTY);

   return wkc;
}

/** APRMW "auto increment address read, multiple write" primitive. Blocking.
 *
 * @param[in] port        = port context struct
 * @param[in]  ADP        = Address Position, each slave ++, slave that has 0 reads,
 *                          following slaves write.
 * @param[in]  ADO        = Address Offset, slave memory address
 * @param[in]  length     = length of databuffer
 * @param[out] data       = databuffer to put slave data in
 * @param[in]  timeout    = timeout in us, standard is EC_TIMEOUTRET
 * @return Workcounter or EC_NOFRAME
 */
int ecx_ARMW(ecx_portt *port, uint16 ADP, uint16 ADO, uint16 length, void *data, int timeout)
{
   int wkc;
   uint8 idx;

   idx = ecx_getindex(port);
   ecx_setupdatagram(port, &(port->txbuf[idx]), EC_CMD_ARMW, idx, ADP, ADO, length, data);
   wkc = ecx_srconfirm(port, idx, timeout);
   if (wkc > 0)
   {
      memcpy(data, &(port->rxbuf[idx][EC_HEADERSIZE]), length);
   }
   ecx_setbufstat(port, idx, EC_BUF_EMPTY);

   return wkc;
}

/** FPRMW "configured address read, multiple write" primitive. Blocking.
 *
 * @param[in] port        = port context struct
 * @param[in]  ADP        = Address Position, slave that has address reads,
 *                          following slaves write.
 * @param[in]  ADO        = Address Offset, slave memory address
 * @param[in]  length     = length of databuffer
 * @param[out] data       = databuffer to put slave data in
 * @param[in]  timeout    = timeout in us, standard is EC_TIMEOUTRET
 * @return Workcounter or EC_NOFRAME
 */
int ecx_FRMW(ecx_portt *port, uint16 ADP, uint16 ADO, uint16 length, void *data, int timeout)
{
   int wkc;
   uint8 idx;

   idx = ecx_getindex(port);
   ecx_setupdatagram(port, &(port->txbuf[idx]), EC_CMD_FRMW, idx, ADP, ADO, length, data);
   wkc = ecx_srconfirm(port, idx, timeout);
   if (wkc > 0)
   {
      memcpy(data, &(port->rxbuf[idx][EC_HEADERSIZE]), length);
   }
   ecx_setbufstat(port, idx, EC_BUF_EMPTY);

   return wkc;
}

/** APRDw "auto increment address read" word return primitive. Blocking.
 *
 * @param[in] port        = port context struct
 * @param[in] ADP         = Address Position, each slave ++, slave that has 0 reads.
 * @param[in] ADO         = Address Offset, slave memory address
 * @param[in] timeout     = timeout in us, standard is EC_TIMEOUTRET
 * @return word data from slave
 */
uint16 ecx_APRDw(ecx_portt *port, uint16 ADP, uint16 ADO, int timeout)
{
   uint16 w;

   w = 0;
   ecx_APRD(port, ADP, ADO, sizeof(w), &w, timeout);

   return w;
}

/** FPRD "configured address read" primitive. Blocking.
 *
 * @param[in] port        = port context struct
 * @param[in]  ADP        = Address Position, slave that has address reads.
 * @param[in]  ADO        = Address Offset, slave memory address
 * @param[in]  length     = length of databuffer
 * @param[out] data       = databuffer to put slave data in
 * @param[in]  timeout    = timeout in us, standard is EC_TIMEOUTRET
 * @return Workcounter or EC_NOFRAME
 */
int ecx_FPRD(ecx_portt *port, uint16 ADP, uint16 ADO, uint16 length, void *data, int timeout)
{
   int wkc;
   uint8 idx;

   idx = ecx_getindex(port);
   ecx_setupdatagram(port, &(port->txbuf[idx]), EC_CMD_FPRD, idx, ADP, ADO, length, data);
   wkc = ecx_srconfirm(port, idx, timeout);
   if (wkc > 0)
   {
      memcpy(data, &(port->rxbuf[idx][EC_HEADERSIZE]), length);
   }
   ecx_setbufstat(port, idx, EC_BUF_EMPTY);

   return wkc;
}

/** FPRDw "configured address read" word return primitive. Blocking.
 *
 * @param[in] port        = port context struct
 * @param[in] ADP         = Address Position, slave that has address reads.
 * @param[in] ADO         = Address Offset, slave memory address
 * @param[in] timeout     = timeout in us, standard is EC_TIMEOUTRET
 * @return word data from slave
 */
uint16 ecx_FPRDw(ecx_portt *port, uint16 ADP, uint16 ADO, int timeout)
{
   uint16 w;

   w = 0;
   ecx_FPRD(port, ADP, ADO, sizeof(w), &w, timeout);
   return w;
}

/** APWR "auto increment address write" primitive. Blocking.
 *
 * @param[in] port        = port context struct
 * @param[in] ADP         = Address Position, each slave ++, slave that has 0 writes.
 * @param[in] ADO         = Address Offset, slave memory address
 * @param[in] length      = length of databuffer
 * @param[in] data        = databuffer to write to slave.
 * @param[in] timeout     = timeout in us, standard is EC_TIMEOUTRET
 * @return Workcounter or EC_NOFRAME
 */
int ecx_APWR(ecx_portt *port, uint16 ADP, uint16 ADO, uint16 length, void *data, int timeout)
{
   uint8 idx;
   int wkc;

   idx = ecx_getindex(port);
   ecx_setupdatagram(port, &(port->txbuf[idx]), EC_CMD_APWR, idx, ADP, ADO, length, data);
   wkc = ecx_srconfirm(port, idx, timeout);
   ecx_setbufstat(port, idx, EC_BUF_EMPTY);

   return wkc;
}

/** APWRw "auto increment address write" word primitive. Blocking.
 *
 * @param[in] port        = port context struct
 * @param[in] ADP         = Address Position, each slave ++, slave that has 0 writes.
 * @param[in] ADO         = Address Offset, slave memory address
 * @param[in] data        = word data to write to slave.
 * @param[in] timeout     = timeout in us, standard is EC_TIMEOUTRET
 * @return Workcounter or EC_NOFRAME
 */
int ecx_APWRw(ecx_portt *port, uint16 ADP, uint16 ADO, uint16 data, int timeout)
{
   return ecx_APWR(port, ADP, ADO, sizeof(data), &data, timeout);
}

/** FPWR "configured address write" primitive. Blocking.
 *
 * @param[in] port        = port context struct
 * @param[in] ADP         = Address Position, slave that has address writes.
 * @param[in] ADO         = Address Offset, slave memory address
 * @param[in] length      = length of databuffer
 * @param[in] data        = databuffer to write to slave.
 * @param[in] timeout     = timeout in us, standard is EC_TIMEOUTRET
 * @return Workcounter or EC_NOFRAME
 */
int ecx_FPWR(ecx_portt *port, uint16 ADP, uint16 ADO, uint16 length, void *data, int timeout)
{
   int wkc;
   uint8 idx;

   idx = ecx_getindex(port);
   ecx_setupdatagram(port, &(port->txbuf[idx]), EC_CMD_FPWR, idx, ADP, ADO, length, data);
   wkc = ecx_srconfirm(port, idx, timeout);
   ecx_setbufstat(port, idx, EC_BUF_EMPTY);

   return wkc;
}

/** FPWR "configured address write" primitive. Blocking. Modified for mailbox servo
 *
 * @param[in] port        = port context struct
 * @param[in] ADP         = Address Position, slave that has address writes.
 * @param[in] ADO         = Address Offset, slave memory address
 * @param[in] length      = length of databuffer
 * @param[in] data        = databuffer to write to slave.
 * @param[in] timeout     = timeout in us, standard is EC_TIMEOUTRET
 * @return Workcounter or EC_NOFRAME
 */
int ecx_FPWR_mbxmod(ecx_portt *port, uint16 ADP, uint16 ADO, uint16 length, void *data, int timeout)
{
   int wkc;
   uint8 idx;
   uint8 nodata = 0;

   idx = ecx_getindex(port);
   ecx_setupdatagram(port, &(port->txbuf[idx]), EC_CMD_FPWR, idx, ADP, ADO, length, data);
   ecx_adddatagram(port, &(port->txbuf[idx]), EC_CMD_FPWR, idx, FALSE, ADP, 0x10ff, sizeof(nodata), &nodata);
   wkc = ecx_srconfirm(port, idx, timeout);
   ecx_setbufstat(port, idx, EC_BUF_EMPTY);

   return wkc;
}

/** FPWR "configured address write" primitive. Blocking.
 *
 * @param[in] port        = port context struct
 * @param[in] ADP         = Address Position, slave that has address writes.
 * @param[in] ADO         = Address Offset, slave memory address
 * @param[in] data        = word to write to slave.
 * @param[in] timeout     = timeout in us, standard is EC_TIMEOUTRET
 * @return Workcounter or EC_NOFRAME
 */
int ecx_FPWRw(ecx_portt *port, uint16 ADP, uint16 ADO, uint16 data, int timeout)
{
   return ecx_FPWR(port, ADP, ADO, sizeof(data), &data, timeout);
}

/** LRW "logical memory read / write" primitive. Blocking.
 *
 * @param[in] port        = port context struct
 * @param[in]     LogAdr  = Logical memory address
 * @param[in]     length  = length of databuffer
 * @param[in,out] data    = databuffer to write to and read from slave.
 * @param[in]     timeout = timeout in us, standard is EC_TIMEOUTRET
 * @return Workcounter or EC_NOFRAME
 */
int ecx_LRW(ecx_portt *port, uint32 LogAdr, uint16 length, void *data, int timeout)
{
   uint8 idx;
   int wkc;

   idx = ecx_getindex(port);
   ecx_setupdatagram(port, &(port->txbuf[idx]), EC_CMD_LRW, idx, LO_WORD(LogAdr), HI_WORD(LogAdr), length, data);
   wkc = ecx_srconfirm(port, idx, timeout);
   if ((wkc > 0) && (port->rxbuf[idx][EC_CMDOFFSET] == EC_CMD_LRW))
   {
      memcpy(data, &(port->rxbuf[idx][EC_HEADERSIZE]), length);
   }
   ecx_setbufstat(port, idx, EC_BUF_EMPTY);

   return wkc;
}

/** LRD "logical memory read" primitive. Blocking.
 *
 * @param[in] port        = port context struct
 * @param[in]  LogAdr     = Logical memory address
 * @param[in]  length     = length of bytes to read from slave.
 * @param[out] data       = databuffer to read from slave.
 * @param[in]  timeout    = timeout in us, standard is EC_TIMEOUTRET
 * @return Workcounter or EC_NOFRAME
 */
int ecx_LRD(ecx_portt *port, uint32 LogAdr, uint16 length, void *data, int timeout)
{
   uint8 idx;
   int wkc;

   idx = ecx_getindex(port);
   ecx_setupdatagram(port, &(port->txbuf[idx]), EC_CMD_LRD, idx, LO_WORD(LogAdr), HI_WORD(LogAdr), length, data);
   wkc = ecx_srconfirm(port, idx, timeout);
   if ((wkc > 0) && (port->rxbuf[idx][EC_CMDOFFSET]==EC_CMD_LRD))
   {
      memcpy(data, &(port->rxbuf[idx][EC_HEADERSIZE]), length);
   }
   ecx_setbufstat(port, idx, EC_BUF_EMPTY);

   return wkc;
}

/** LWR "logical memory write" primitive. Blocking.
 *
 * @param[in] port        = port context struct
 * @param[in] LogAdr      = Logical memory address
 * @param[in] length      = length of databuffer
 * @param[in] data        = databuffer to write to slave.
 * @param[in] timeout     = timeout in us, standard is EC_TIMEOUTRET
 * @return Workcounter or EC_NOFRAME
 */
int ecx_LWR(ecx_portt *port, uint32 LogAdr, uint16 length, void *data, int timeout)
{
   uint8 idx;
   int wkc;

   idx = ecx_getindex(port);
   ecx_setupdatagram(port, &(port->txbuf[idx]), EC_CMD_LWR, idx, LO_WORD(LogAdr), HI_WORD(LogAdr), length, data);
   wkc = ecx_srconfirm(port, idx, timeout);
   ecx_setbufstat(port, idx, EC_BUF_EMPTY);

   return wkc;
}

/** LRW "logical memory read / write" primitive plus Clock Distribution. Blocking.
 * Frame consists of two datagrams, one LRW and one FPRMW.
 *
 * @param[in] port        = port context struct
 * @param[in]     LogAdr  = Logical memory address
 * @param[in]     length  = length of databuffer
 * @param[in,out] data    = databuffer to write to and read from slave.
 * @param[in]     DCrs    = Distributed Clock reference slave address.
 * @param[out]    DCtime  = DC time read from reference slave.
 * @param[in]     timeout = timeout in us, standard is EC_TIMEOUTRET
 * @return Workcounter or EC_NOFRAME
 */
int ecx_LRWDC(ecx_portt *port, uint32 LogAdr, uint16 length, void *data, uint16 DCrs, int64 *DCtime, int timeout)
{
   uint16 DCtO;
   uint8 idx;
   int wkc;
   uint64 DCtE;

   idx = ecx_getindex(port);
   /* LRW in first datagram */
   ecx_setupdatagram(port, &(port->txbuf[idx]), EC_CMD_LRW, idx, LO_WORD(LogAdr), HI_WORD(LogAdr), length, data);
   /* FPRMW in second datagram */
   DCtE = htoell(*DCtime);
   DCtO = ecx_adddatagram(port, &(port->txbuf[idx]), EC_CMD_FRMW, idx, FALSE, DCrs, ECT_REG_DCSYSTIME, sizeof(DCtime), &DCtE);
   wkc = ecx_srconfirm(port, idx, timeout);
   if ((wkc > 0) && (port->rxbuf[idx][EC_CMDOFFSET] == EC_CMD_LRW))
   {
      memcpy(data, &(port->rxbuf[idx][EC_HEADERSIZE]), length);
      memcpy(&wkc, &(port->rxbuf[idx][EC_HEADERSIZE + length]), EC_WKCSIZE);
      memcpy(&DCtE, &(port->rxbuf[idx][DCtO]), sizeof(*DCtime));
      *DCtime = etohll(DCtE);
   }
   ecx_setbufstat(port, idx, EC_BUF_EMPTY);

   return wkc;
}

int ecx_5cmds_nop(ecx_portt *port, int timeout) {
   uint8 idx;
   int wkc;

   uint8 null1 = 0;
   uint16 null2 = 0;
   uint32 null4 = 0;
   uint32 time;
   ec_timet mastertime = osal_current_time();
   mastertime.sec -= 946684800UL;  /* EtherCAT uses 2000-01-01 as epoch start instead of 1970-01-01 */
   uint64 nanoseconds = (((uint64)mastertime.sec * 1000000) + (uint64)mastertime.usec) * 1000;
   time = nanoseconds & 0xffffffff;

   idx = ecx_getindex(port);
   ecx_setupdatagram(port, &(port->txbuf[idx]), EC_CMD_NOP, idx, 0, 0x900, 4, &null4);
   // DC SysTime L (lower half)
   ecx_adddatagram(port, &(port->txbuf[idx]), EC_CMD_ARMW, idx, TRUE, 0, 0x910, 4, &time);
   ecx_adddatagram(port, &(port->txbuf[idx]), EC_CMD_LRD, idx, TRUE, 0, 0x900, 1, &null1);
   // Outputs (6 bytes (pad to 10)): Controlword (2 bytes) + Target position (4 bytes) + padding (4 bytes)
   struct outputs {
      uint16 status_word;
      uint32 target_pos;
      uint32 padding;
   };
   struct outputs data;
   data.status_word = 6;
   data.target_pos = 0x0;
   data.padding = 0;

   ecx_adddatagram(port, &(port->txbuf[idx]), EC_CMD_NOP, idx, TRUE, 0, 0x100, 10, &data);
   // Status
   ecx_adddatagram(port, &(port->txbuf[idx]), EC_CMD_BRD, idx, FALSE, 0, 0x130, 2, &null2);
   wkc = ecx_srconfirm(port, idx, timeout);
   ecx_setbufstat(port, idx, EC_BUF_EMPTY);

   return wkc;
}

int ecx_5cmds_lrw(ecx_portt *port, uint16 controlword, uint32 target_pos, int timeout) {
   uint8 idx;
   int wkc;

   uint8 null1 = 0;
   uint16 null2 = 0;
   uint32 null4 = 0;
   uint32 time;
   ec_timet mastertime = osal_current_time();
   mastertime.sec -= 946684800UL;  /* EtherCAT uses 2000-01-01 as epoch start instead of 1970-01-01 */
   uint64 nanoseconds = (((uint64)mastertime.sec * 1000000) + (uint64)mastertime.usec) * 1000;
   time = nanoseconds & 0xffffffff;

   idx = ecx_getindex(port);
   ecx_setupdatagram(port, &(port->txbuf[idx]), EC_CMD_NOP, idx, 0, 0x900, 4, &null4);
   // DC SysTime L (lower half)
   ecx_adddatagram(port, &(port->txbuf[idx]), EC_CMD_ARMW, idx, TRUE, 0, 0x910, 4, &time);
   ecx_adddatagram(port, &(port->txbuf[idx]), EC_CMD_LRD, idx, TRUE, 0, 0x900, 1, &null1);

   // Set outputs
   our_outputs data;
   data.controlword = controlword;
   data.target_pos = target_pos;
   data.padding = 0;

   ecx_adddatagram(port, &(port->txbuf[idx]), EC_CMD_LRW, idx, TRUE, 0, 0x100, 10, &data);
   // Status
   ecx_adddatagram(port, &(port->txbuf[idx]), EC_CMD_BRD, idx, FALSE, 0, 0x130, 2, &null2);
   wkc = ecx_srconfirm(port, idx, timeout);
   ecx_setbufstat(port, idx, EC_BUF_EMPTY);

   uint8* rcvd = port->rxbuf[idx];
   // printf("------------------------------------\n");
   // for (int i = 0; i < 100; i++) {
   //    printf("Received %d: %x\n", i, rcvd[i]);
   // }

   // Get inputs
   our_inputs* in;
   in = (our_inputs *)&rcvd[57]; // Start of inputs

   printf("Inputs: %f - %#x - %d\n", in->position/10000.0, in->statusword, in->erroract);

   return wkc;
}

#ifdef EC_VER1
int ec_setupdatagram(void *frame, uint8 com, uint8 idx, uint16 ADP, uint16 ADO, uint16 length, void *data)
{
   assert(frame ==  NULL); // If this triggers change back code below...
   // FIX: fix datagram
   return ecx_setupdatagram (&ecx_port, &(ecx_port.txbuf[idx]), com, idx, ADP, ADO, length, data);
}

uint16 ec_adddatagram (void *frame, uint8 com, uint8 idx, boolean more, uint16 ADP, uint16 ADO, uint16 length, void *data)
{
   assert(frame ==  NULL); // If this triggers change back code below...
   // FIX: fix datagram
   return ecx_adddatagram (&ecx_port, &(ecx_port.txbuf[idx]), com, idx, more, ADP, ADO, length, data);
}

int ec_BWR(uint16 ADP, uint16 ADO, uint16 length, void *data, int timeout)
{
   return ecx_BWR (&ecx_port, ADP, ADO, length, data, timeout);
}

int ec_BRD(uint16 ADP, uint16 ADO, uint16 length, void *data, int timeout)
{
   return ecx_BRD(&ecx_port, ADP, ADO, length, data, timeout);
}

int ec_APRD(uint16 ADP, uint16 ADO, uint16 length, void *data, int timeout)
{
   return ecx_APRD(&ecx_port, ADP, ADO, length, data, timeout);
}

int ec_ARMW(uint16 ADP, uint16 ADO, uint16 length, void *data, int timeout)
{
   return ecx_ARMW(&ecx_port, ADP, ADO, length, data, timeout);
}

int ec_FRMW(uint16 ADP, uint16 ADO, uint16 length, void *data, int timeout)
{
   return ecx_FRMW(&ecx_port, ADP, ADO, length, data, timeout);
}

uint16 ec_APRDw(uint16 ADP, uint16 ADO, int timeout)
{
   uint16 w;

   w = 0;
   ec_APRD(ADP, ADO, sizeof(w), &w, timeout);

   return w;
}

int ec_FPRD(uint16 ADP, uint16 ADO, uint16 length, void *data, int timeout)
{
   return ecx_FPRD(&ecx_port, ADP, ADO, length, data, timeout);
}

uint16 ec_FPRDw(uint16 ADP, uint16 ADO, int timeout)
{
   uint16 w;

   w = 0;
   ec_FPRD(ADP, ADO, sizeof(w), &w, timeout);
   return w;
}

int ec_APWR(uint16 ADP, uint16 ADO, uint16 length, void *data, int timeout)
{
   return ecx_APWR(&ecx_port, ADP, ADO, length, data, timeout);
}

int ec_APWRw(uint16 ADP, uint16 ADO, uint16 data, int timeout)
{
   return ec_APWR(ADP, ADO, sizeof(data), &data, timeout);
}

int ec_FPWR(uint16 ADP, uint16 ADO, uint16 length, void *data, int timeout)
{
   return ecx_FPWR(&ecx_port, ADP, ADO, length, data, timeout);
}

int ec_FPWRw(uint16 ADP, uint16 ADO, uint16 data, int timeout)
{
   return ec_FPWR(ADP, ADO, sizeof(data), &data, timeout);
}

int ec_LRW(uint32 LogAdr, uint16 length, void *data, int timeout)
{
   return ecx_LRW(&ecx_port, LogAdr, length, data, timeout);
}

int ec_LRD(uint32 LogAdr, uint16 length, void *data, int timeout)
{
   return ecx_LRD(&ecx_port, LogAdr, length, data, timeout);
}

int ec_LWR(uint32 LogAdr, uint16 length, void *data, int timeout)
{
   return ecx_LWR(&ecx_port, LogAdr, length, data, timeout);
}

int ec_LRWDC(uint32 LogAdr, uint16 length, void *data, uint16 DCrs, int64 *DCtime, int timeout)
{
   return ecx_LRWDC(&ecx_port, LogAdr, length, data, DCrs, DCtime, timeout);
}

int ec_5cmds_nop(int timeout) {
   return ecx_5cmds_nop(&ecx_port, timeout);
}
int ec_5cmds_lrw(uint16 controlword, uint32 target_pos, int timeout) {
   return ecx_5cmds_lrw(&ecx_port, controlword, target_pos, timeout);
}
#endif
