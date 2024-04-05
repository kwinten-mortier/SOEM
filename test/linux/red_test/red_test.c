/** \file
 * \brief Example code for Simple Open EtherCAT master
 *
 * Usage : red_test [ifname1] [ifname2] [cycletime]
 * ifname is NIC interface, f.e. eth0
 * cycletime in us, f.e. 500
 *
 * This is a redundancy test.
 *
 * (c)Arthur Ketels 2008
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <sched.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include <inttypes.h>

#include "ethercat.h"

#define NSEC_PER_SEC 1000000000
#define EC_TIMEOUTMON 500

struct sched_param schedp;
char IOmap[4096];
pthread_t thread1, thread2;
struct timeval tv, t1, t2;
int dorun = 0;
int deltat, tmax = 0;
int64 toff, gl_delta;
int DCdiff;
int os;
uint8 ob;
uint16 ob2;
uint8 *digout = 0;
int expectedWKC;
boolean needlf;
volatile int wkc;
boolean inOP;
uint8 currentgroup = 0;

// Run all startup/init commands (for PREOP->SAFEOP), should return 1
int configservo(uint16 slave) {
   uint8 u8val;
   uint16 u16val;
   uint32 u32val;
   int retval = 0;

   // Motor Settings

   // Set max current
   u32val = 48000;
   retval += ec_SDOwrite(slave, 0x8011, 0x11, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTSAFE);
   printf("0x8011:11: retval = %d\n", retval);

   // Set rated current
   u32val = 8000;
   retval += ec_SDOwrite(slave, 0x8011, 0x12, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTSAFE);
   printf("0x8011:12: retval = %d\n", retval);

   // Set motor pole pairs
   u8val = 3;
   retval += ec_SDOwrite(slave, 0x8011, 0x13, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTSAFE);
   printf("0x8011:13: retval = %d\n", retval);

   // Set torque constant
   u32val = 100;
   retval += ec_SDOwrite(slave, 0x8011, 0x16, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTSAFE);
   printf("0x8011:16: retval = %d\n", retval);
   
   // Set rotor moment of intertia
   u32val = 253;
   retval += ec_SDOwrite(slave, 0x8011, 0x18, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTSAFE);
   printf("0x8011:18: retval = %d\n", retval);

   // Set winding inductance
   u16val = 70;
   retval += ec_SDOwrite(slave, 0x8011, 0x19, FALSE, sizeof(u16val), &u16val, EC_TIMEOUTSAFE);
   printf("0x8011:19: retval = %d\n", retval);

   // Set motor speed limitation
   u32val = 12000;
   retval += ec_SDOwrite(slave, 0x8011, 0x1b, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTSAFE);
   printf("0x8011:1b: retval = %d\n", retval);

   // Set motor temperature warn level
   u16val = 1200;
   retval += ec_SDOwrite(slave, 0x8011, 0x2b, FALSE, sizeof(u16val), &u16val, EC_TIMEOUTSAFE);
   printf("0x8011:2b: retval = %d\n", retval);

   // Set motor temperature error level
   u16val = 1400;
   retval += ec_SDOwrite(slave, 0x8011, 0x2c, FALSE, sizeof(u16val), &u16val, EC_TIMEOUTSAFE);
   printf("0x8011:2c: retval = %d\n", retval);

   // Set motor thermal time constant
   u16val = 1050;
   retval += ec_SDOwrite(slave, 0x8011, 0x2d, FALSE, sizeof(u16val), &u16val, EC_TIMEOUTSAFE);
   printf("0x8011:2d: retval = %d\n", retval);

   // Set winding resistance
   u32val = 360;
   retval += ec_SDOwrite(slave, 0x8011, 0x30, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTSAFE);
   printf("0x8011:30: retval = %d\n", retval);

   // Set voltage constant
   u32val = 5500;
   retval += ec_SDOwrite(slave, 0x8011, 0x31, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTSAFE);
   printf("0x8011:31: retval = %d\n", retval);

   // Set commutation offset
   u16val = 270;
   retval += ec_SDOwrite(slave, 0x8011, 0x15, FALSE, sizeof(u16val), &u16val, EC_TIMEOUTSAFE);
   printf("0x8011:15: retval = %d\n", retval);

   // Outputs

   // Set torque limitation
   u16val = 2250;
   retval += ec_SDOwrite(slave, 0x7010, 0x0b, FALSE, sizeof(u16val), &u16val, EC_TIMEOUTSAFE);
   printf("0x7010:0b: retval = %d\n", retval);

   // Amplifier Settings

   // Set current loop P gain
   u16val = 39;
   retval += ec_SDOwrite(slave, 0x8010, 0x13, FALSE, sizeof(u16val), &u16val, EC_TIMEOUTSAFE);
   printf("0x8010:13: retval = %d\n", retval);

   // Set current loop I gain
   u16val = 19;
   retval += ec_SDOwrite(slave, 0x8010, 0x12, FALSE, sizeof(u16val), &u16val, EC_TIMEOUTSAFE);
   printf("0x8010:12: retval = %d\n", retval);

   // Set velocity loop P gain
   u32val = 224;
   retval += ec_SDOwrite(slave, 0x8010, 0x15, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTSAFE);
   printf("0x8010:15: retval = %d\n", retval);

   // Set velocity loop I gain
   u32val = 150;
   retval += ec_SDOwrite(slave, 0x8010, 0x14, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTSAFE);
   printf("0x8010:14: retval = %d\n", retval);

   // FB Settings

   // Set observer feed-forward
   u8val = 100;
   retval += ec_SDOwrite(slave, 0x8000, 0x15, FALSE, sizeof(u8val), &u8val, EC_TIMEOUTSAFE);
   printf("0x8000:15: retval = %d\n", retval);

   // Amplifier Settings

   // Set nominal DC link voltage
   u32val = 24000;
   retval += ec_SDOwrite(slave, 0x8010, 0x19, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTSAFE);
   // TODO: Again, same FPRD command seems to be needed to read in data from the slave
   // Same for the FPRD messages with Ado = 0x80d and Ado = 0x805
   printf("0x8010:19: retval = %d\n", retval);
   return retval;
}

// Should return 4
int setupdc(uint16 slave)
{
   int retval = 0;
   // Register SYNC0 and SYNC1 cycle time
   uint32 cycletime_01[2] = {0xf424, 0x1d905c};
   retval += ec_FPWR(ec_slave[slave].configadr, 0x9a0, sizeof(cycletime_01), &cycletime_01, EC_TIMEOUTRET);
   printf("0x9a0: retval = %d\n", retval);

   // Set DC StartTime0
   // Get the current time
   ec_timet mastertime = osal_current_time();
   mastertime.sec -= 946684800UL;  /* EtherCAT uses 2000-01-01 as epoch start instead of 1970-01-01 */
   uint64 DC_starttime_0 = (((uint64)mastertime.sec * 1000000) + (uint64)mastertime.usec) * 1000;
   uint8 idx = ec_getindex();
   ec_setupdatagram(NULL, EC_CMD_FPWR, idx, ec_slave[slave].configadr, 0x990, sizeof(DC_starttime_0), &DC_starttime_0);
   uint64 null8 = 0;
   ec_adddatagram(NULL, EC_CMD_FPRD, idx, FALSE, ec_slave[slave].configadr, 0x910, sizeof(null8), &null8);
   retval += ec_srconfirm(idx, EC_TIMEOUTRET);
   ec_setbufstat(idx, EC_BUF_EMPTY);
   printf("0x990 and 0x910: retval = %d\n", retval);

   // Set DC Cycle Unit and Activation
   uint8 DC_CU_ACT[2] = {0, 0x7};
   retval += ec_FPWR(ec_slave[slave].configadr, 0x980, sizeof(DC_CU_ACT), &DC_CU_ACT, EC_TIMEOUTRET);
   printf("0x980: retval = %d\n", retval);

   // Set DC Latch0 and Latch1
   uint8 DC_Latch_01[2] = {0, 0};
   retval += ec_FPWR(ec_slave[slave].configadr, 0x9a8, sizeof(DC_Latch_01), &DC_Latch_01, EC_TIMEOUTRET);
   printf("0x9a8: retval = %d\n", retval);

   return retval;
}


// I think separate function might be needed for each phase of the setup, but not sure
// Init commands seem to be called automatically, but not sure if they are called at the right time
// Some init commands are called at the right time, but not all of them???
// Init commands: I->P vs P->S vs S->O
// TODO: Init commands (see IO->Device->EtherCAT->Advanced Settings...->General->Init Commands)


// Startup commands
int servo_setup(uint16 slave)
{
   int retval;
   uint16 u16val;
   uint32 u32val;
   
   retval = 0;

   /* set Revision number*/
   u32val = 0x00110000;
   retval += ec_SDOwrite(slave, 0xf081, 0x01, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTSAFE);
   // TODO: FPRD to let slave fill in data of the SDO. Answer is Mbx(CoE SDO) so maybe ec_SDOread? Ado = 0x1100 (Start FMMU/SM1) / Mailbox In / length = 256
   // For Ado = 0x1100, also look at section 3.8.4 of "the EtherCAT thesis" 
   // Why do we get so much FPRD messages with Ado = 0x80d (Status Register SM1 (SM1 = Mailbox In (Slave to Master)))?
   // We also get 1 message with Ado = 0x805 (Status Register SM0 (SMO = Mailbox Out (Master to Slave)))?

   printf("0xf081: retval = %d\n", retval); // Why do we get -5??? -> See Comment in ec_SDOwrite in ethercat.c (print wkc for debugging)
                                    // retval = -5 means return value request timeout
                                    // We get this for every startup command, so maybe the slave is not in the right state?
   /* set Flags */
   u16val = 0x0001;
   retval += ec_SDOwrite(slave, 0x10f3, 0x05, FALSE, sizeof(u16val), &u16val, EC_TIMEOUTSAFE);
   // TODO: FPRD to let slave fill in data of the SDO. Answer is Mbx(CoE SDO) so maybe ec_SDOread? Ado = 0x1100 (Start FMMU/SM1) / Mailbox In / length = 256
   // Why do we get so much FPRD messages with Ado = 0x80d (Status Register SM1 (SM1 = Mailbox In (Slave to Master)))?
   // We also get 1 message with Ado = 0x805 (Status Register SM0 (SMO = Mailbox Out (Master to Slave)))?
   printf("0x10f3: retval = %d\n", retval);
   
   /* Map velocity(?) PDO assignment via Complete Access*/
   uint16 map_1c12[3] = {0x0002, 0x1600, 0x1606};
   uint16 map_1c13[4] = {0x0003, 0x1a00, 0x1a01, 0x1a06};
   retval += ec_SDOwrite(slave, 0x1c12, 0x00, TRUE, sizeof(map_1c12), &map_1c12, EC_TIMEOUTSAFE);
   printf("0x1c12: retval = %d\n", retval);
   retval += ec_SDOwrite(slave, 0x1c13, 0x00, TRUE, sizeof(map_1c13), &map_1c13, EC_TIMEOUTSAFE);
   // TODO: Again, same FPRD command seems to be needed to read in data from the slave
   // Same for the FPRD messages with Ado = 0x80d and Ado = 0x805
   printf("0x1c13: retval = %d\n", retval);

   // Configure servo drive
   retval += configservo(slave);

   // // Setup DC
   // retval += setupdc(slave);

   // // Set syncmanagers
   // typedef struct {
   //    uint16 start;
   //    uint16 length;
   //    uint16 controlstatus;
   //    uint16 enable;
   // } SM;
   // // Set SM0
   // SM SM0;
   // SM0.start = 0x1200;
   // SM0.length = 0x0006;
   // SM0.controlstatus = 0x0024;
   // SM0.enable = 0x0001;
   // retval += ec_FPWR(ec_slave[slave].configadr, 0x810, sizeof(SM), &SM0, EC_TIMEOUTRET);
   // printf("0x810: retval = %d\n", retval);
   
   // // Set SM1
   // SM SM1;
   // SM1.start = 0x1400;
   // SM1.length = 0x000a;
   // SM1.controlstatus = 0x0020;
   // SM1.enable = 0x0001;
   // retval += ec_FPWR(ec_slave[slave].configadr, 0x818, sizeof(SM), &SM1, EC_TIMEOUTRET);
   // printf("0x818: retval = %d\n", retval);


   // typedef struct {
   //    uint32 logstart;
   //    uint16 loglength;
   //    uint8 startbit;
   //    uint8 endbit;
   //    uint16 physstart;
   //    uint8 physstartbit;
   //    uint8 type;
   //    uint8 activate;
   // } __attribute__((packed)) FMMU;
   // // Set FMMU0 (maps outputs) 0x600
   // FMMU FMMU0;
   // FMMU0.logstart = 0x01000000;
   // FMMU0.loglength = 0x0006;
   // FMMU0.startbit = 0;
   // FMMU0.endbit = 7;
   // FMMU0.physstart = 0x1200;
   // FMMU0.physstartbit = 0;
   // FMMU0.type = 0x02;
   // FMMU0.activate = 0x01;
   // retval += ec_FPWR(ec_slave[slave].configadr, 0x600, sizeof(FMMU), &FMMU0, EC_TIMEOUTRET);
   // printf("0x600: retval = %d\n", retval);
   
   // // Set FMMU1 (maps inputs) 0x610
   // FMMU FMMU1;
   // FMMU1.logstart = 0x01000000;
   // FMMU1.loglength = 0x000a;
   // FMMU1.startbit = 0;
   // FMMU1.endbit = 7;
   // FMMU1.physstart = 0x1400;
   // FMMU1.physstartbit = 0;
   // FMMU1.type = 0x01;
   // FMMU1.activate = 0x01;
   // retval += ec_FPWR(ec_slave[slave].configadr, 0x610, sizeof(FMMU), &FMMU1, EC_TIMEOUTRET);
   // printf("0x610: retval = %d\n", retval);

   printf("Servo slave %d set, retval = %d\n", slave, retval);
   return retval;
}

void redtest(char *ifname, char *ifname2)
{
   int cnt, i, j, oloop, iloop;

   printf("Starting Redundant test\n");

   /* initialise SOEM, bind socket to ifname */
   (void) ifname2;
//   if (ec_init_redundant(ifname, ifname2))
   if (ec_init(ifname))
   {
      printf("ec_init on %s succeeded.\n",ifname);
      /* find and auto-config slaves */
      if ( ec_config(FALSE, &IOmap) > 0 )
      {
         printf("%d slaves found and configured.\n",ec_slavecount);
         /* wait for all slaves to reach SAFE_OP state */
         ec_statecheck(0, EC_STATE_SAFE_OP,  EC_TIMEOUTSTATE);

         ec_slave[1].PO2SOconfig = servo_setup;

         /* configure DC options for every DC capable slave found in the list */
         ec_configdc();
         ec_dcsync01(1, TRUE, 62500, 0, 0);

         /* read indevidual slave state and store in ec_slave[] */
         ec_readstate();
         for(cnt = 1; cnt <= ec_slavecount ; cnt++)
         {
            printf("Slave:%d Name:%s Output size:%3dbits Input size:%3dbits State:%2d delay:%d.%d\n",
                  cnt, ec_slave[cnt].name, ec_slave[cnt].Obits, ec_slave[cnt].Ibits,
                  ec_slave[cnt].state, (int)ec_slave[cnt].pdelay, ec_slave[cnt].hasdc);
            printf("         Out:%p,%4d In:%p,%4d\n",
                  ec_slave[cnt].outputs, ec_slave[cnt].Obytes, ec_slave[cnt].inputs, ec_slave[cnt].Ibytes);
            /* check for EL2004 or EL2008 */
            if( !digout && ((ec_slave[cnt].eep_id == 0x0af83052) || (ec_slave[cnt].eep_id == 0x07d83052)))
            {
               digout = ec_slave[cnt].outputs;
            }
         }
         expectedWKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
         printf("Calculated workcounter %d\n", expectedWKC);

         printf("Request operational state for all slaves\n");
         ec_slave[0].state = EC_STATE_OPERATIONAL;
         /* request OP state for all slaves */
         ec_writestate(0);
         /* activate cyclic process data */
         dorun = 1;
         /* wait for all slaves to reach OP state */
         ec_statecheck(0, EC_STATE_OPERATIONAL,  5 * EC_TIMEOUTSTATE);
         oloop = ec_slave[0].Obytes;
         if ((oloop == 0) && (ec_slave[0].Obits > 0)) oloop = 1;
         if (oloop > 8) oloop = 8;
         iloop = ec_slave[0].Ibytes;
         if ((iloop == 0) && (ec_slave[0].Ibits > 0)) iloop = 1;
         if (iloop > 8) iloop = 8;
         if (ec_slave[0].state == EC_STATE_OPERATIONAL )
         {
            printf("Operational state reached for all slaves.\n");
            inOP = TRUE;
            /* acyclic loop 5000 x 20ms = 10s */
            for(i = 1; i <= 5000; i++)
            {
               printf("Processdata cycle %5d , Wck %3d, DCtime %12"PRId64", dt %12"PRId64", O:",
                  dorun, wkc , ec_DCtime, gl_delta);
               for(j = 0 ; j < oloop; j++)
               {
                  printf(" %2.2x", *(ec_slave[0].outputs + j));
               }
               printf(" I:");
               for(j = 0 ; j < iloop; j++)
               {
                  printf(" %2.2x", *(ec_slave[0].inputs + j));
               }
               printf("\r");
               fflush(stdout);
               osal_usleep(20000);
            }
            dorun = 0;
            inOP = FALSE;
         }
         else
         {
            printf("Not all slaves reached operational state.\n");
             ec_readstate();
             for(i = 1; i<=ec_slavecount ; i++)
             {
                 if(ec_slave[i].state != EC_STATE_OPERATIONAL)
                 {
                     printf("Slave %d State=0x%2.2x StatusCode=0x%4.4x : %s\n",
                         i, ec_slave[i].state, ec_slave[i].ALstatuscode, ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
                 }
             }
         }
         printf("Request safe operational state for all slaves\n");
         ec_slave[0].state = EC_STATE_SAFE_OP;
         /* request SAFE_OP state for all slaves */
         ec_writestate(0);
      }
      else
      {
         printf("No slaves found!\n");
      }
      printf("End redundant test, close socket\n");
      /* stop SOEM, close socket */
      ec_close();
   }
   else
   {
      printf("No socket connection on %s\nExcecute as root\n",ifname);
   }
}

/* add ns to timespec */
void add_timespec(struct timespec *ts, int64 addtime)
{
   int64 sec, nsec;

   nsec = addtime % NSEC_PER_SEC;
   sec = (addtime - nsec) / NSEC_PER_SEC;
   ts->tv_sec += sec;
   ts->tv_nsec += nsec;
   if ( ts->tv_nsec >= NSEC_PER_SEC )
   {
      nsec = ts->tv_nsec % NSEC_PER_SEC;
      ts->tv_sec += (ts->tv_nsec - nsec) / NSEC_PER_SEC;
      ts->tv_nsec = nsec;
   }
}

/* PI calculation to get linux time synced to DC time */
void ec_sync(int64 reftime, int64 cycletime , int64 *offsettime)
{
   static int64 integral = 0;
   int64 delta;
   /* set linux sync point 50us later than DC sync, just as example */
   delta = (reftime - 50000) % cycletime;
   if(delta> (cycletime / 2)) { delta= delta - cycletime; }
   if(delta>0){ integral++; }
   if(delta<0){ integral--; }
   *offsettime = -(delta / 100) - (integral / 20);
   gl_delta = delta;
}

/* RT EtherCAT thread */
OSAL_THREAD_FUNC_RT ecatthread(void *ptr)
{
   struct timespec   ts, tleft;
   int ht;
   int64 cycletime;

   clock_gettime(CLOCK_MONOTONIC, &ts);
   ht = (ts.tv_nsec / 1000000) + 1; /* round to nearest ms */
   ts.tv_nsec = ht * 1000000;
   if (ts.tv_nsec >= NSEC_PER_SEC) {
      ts.tv_sec++;
      ts.tv_nsec -= NSEC_PER_SEC;
   }
   cycletime = *(int*)ptr * 1000; /* cycletime in ns */
   toff = 0;
   dorun = 0;
   ec_send_processdata();
   while(1)
   {
      /* calculate next cycle start */
      add_timespec(&ts, cycletime + toff);
      /* wait to cycle start */
      clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, &tleft);
      if (dorun>0)
      {
         wkc = ec_receive_processdata(EC_TIMEOUTRET);

         dorun++;
         /* if we have some digital output, cycle */
         if( digout ) *digout = (uint8) ((dorun / 16) & 0xff);

         if (ec_slave[0].hasdc)
         {
            /* calulate toff to get linux time and DC synced */
            ec_sync(ec_DCtime, cycletime, &toff);
         }
         ec_send_processdata();
      }
   }
}

OSAL_THREAD_FUNC ecatcheck( void *ptr )
{
    int slave;

    (void) ptr;

    while(1)
    {
        if( inOP && ((wkc < expectedWKC) || ec_group[currentgroup].docheckstate))
        {
            if (needlf)
            {
               needlf = FALSE;
               printf("\n");
            }
            /* one ore more slaves are not responding */
            ec_group[currentgroup].docheckstate = FALSE;
            ec_readstate();
            for (slave = 1; slave <= ec_slavecount; slave++)
            {
               if ((ec_slave[slave].group == currentgroup) && (ec_slave[slave].state != EC_STATE_OPERATIONAL))
               {
                  ec_group[currentgroup].docheckstate = TRUE;
                  if (ec_slave[slave].state == (EC_STATE_SAFE_OP + EC_STATE_ERROR))
                  {
                     printf("ERROR : slave %d is in SAFE_OP + ERROR, attempting ack.\n", slave);
                     ec_slave[slave].state = (EC_STATE_SAFE_OP + EC_STATE_ACK);
                     ec_writestate(slave);
                  }
                  else if(ec_slave[slave].state == EC_STATE_SAFE_OP)
                  {
                     printf("WARNING : slave %d is in SAFE_OP, change to OPERATIONAL.\n", slave);
                     ec_slave[slave].state = EC_STATE_OPERATIONAL;
                     ec_writestate(slave);
                  }
                  else if(ec_slave[slave].state > EC_STATE_NONE)
                  {
                     if (ec_reconfig_slave(slave, EC_TIMEOUTMON))
                     {
                        ec_slave[slave].islost = FALSE;
                        printf("MESSAGE : slave %d reconfigured\n",slave);
                     }
                  }
                  else if(!ec_slave[slave].islost)
                  {
                     /* re-check state */
                     ec_statecheck(slave, EC_STATE_OPERATIONAL, EC_TIMEOUTRET);
                     if (ec_slave[slave].state == EC_STATE_NONE)
                     {
                        ec_slave[slave].islost = TRUE;
                        printf("ERROR : slave %d lost\n",slave);
                     }
                  }
               }
               if (ec_slave[slave].islost)
               {
                  if(ec_slave[slave].state == EC_STATE_NONE)
                  {
                     if (ec_recover_slave(slave, EC_TIMEOUTMON))
                     {
                        ec_slave[slave].islost = FALSE;
                        printf("MESSAGE : slave %d recovered\n",slave);
                     }
                  }
                  else
                  {
                     ec_slave[slave].islost = FALSE;
                     printf("MESSAGE : slave %d found\n",slave);
                  }
               }
            }
            if(!ec_group[currentgroup].docheckstate)
               printf("OK : all slaves resumed OPERATIONAL.\n");
        }
        osal_usleep(10000);
    }
}

#define stack64k (64 * 1024)

int main(int argc, char *argv[])
{
   int ctime;

   printf("SOEM (Simple Open EtherCAT Master)\nRedundancy test\n");

   if (argc > 3)
   {
      dorun = 0;
      ctime = atoi(argv[3]);

      /* create RT thread */
      osal_thread_create_rt(&thread1, stack64k * 2, &ecatthread, (void*) &ctime);

      /* create thread to handle slave error handling in OP */
      osal_thread_create(&thread2, stack64k * 4, &ecatcheck, NULL);

      /* start acyclic part */
      redtest(argv[1],argv[2]);
   }
   else
   {
      printf("Usage: red_test ifname1 ifname2 cycletime\nifname = eth0 for example\ncycletime in us\n");
   }

   printf("End program\n");

   return (0);
}
