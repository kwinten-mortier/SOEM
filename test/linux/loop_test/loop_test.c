/** \file
 * \brief Example code for Simple Open EtherCAT master
 *
 * Usage : simple_test [ifname1]
 * ifname is NIC interface, f.e. eth0
 *
 * This is a minimal test.
 *
 * (c)Arthur Ketels 2010 - 2011
 */
#define _GNU_SOURCE  

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <pthread.h>
#include <sched.h>

#include "ethercat.h"

#define EC_TIMEOUTMON 500

char IOmap[4096];
OSAL_THREAD_HANDLE thread1;
int expectedWKC;
boolean needlf;
volatile int wkc;
boolean inOP;
uint8 currentgroup = 0;
boolean forceByteAlignment = FALSE;



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
   
   /* Map velocity PDO assignment via Complete Access*/
   uint16 map_1c12[3] = {0x0002, 0x1600, 0x1606};
   uint16 map_1c13[4] = {0x0003, 0x1a00, 0x1a01, 0x1a06};
   retval += ec_SDOwrite(slave, 0x1c12, 0x00, TRUE, sizeof(map_1c12), &map_1c12, EC_TIMEOUTSAFE);
   printf("0x1c12: retval = %d\n", retval);
   retval += ec_SDOwrite(slave, 0x1c13, 0x00, TRUE, sizeof(map_1c13), &map_1c13, EC_TIMEOUTSAFE);
   // TODO: Again, same FPRD command seems to be needed to read in data from the slave
   // Same for the FPRD messages with Ado = 0x80d and Ado = 0x805
   printf("0x1c13: retval = %d\n", retval);
   
   // /* set nominal DC link voltage */
   u32val = 24000;
   retval += ec_SDOwrite(slave, 0x8010, 0x19, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTSAFE);
   // TODO: Again, same FPRD command seems to be needed to read in data from the slave
   // Same for the FPRD messages with Ado = 0x80d and Ado = 0x805
   printf("0x8010: retval = %d\n", retval);

   // Register SYNC0 and SYNC1 cycle time
   uint32 cycletime_01[2] = {0xf424, 0x1d905c};
   retval += ec_FPWR(ec_slave[slave].configadr, 0x9a0, 8, &cycletime_01, EC_TIMEOUTRET);
   printf("0x9a0: retval = %d\n", retval);

   // Set DC StartTime0
   // Get the current time
   ec_timet mastertime = osal_current_time();
   mastertime.sec -= 946684800UL;  /* EtherCAT uses 2000-01-01 as epoch start instead of 1970-01-01 */
   uint64 DC_starttime_0 = (((uint64)mastertime.sec * 1000000) + (uint64)mastertime.usec) * 1000;
   uint8 idx = ec_getindex();
   ec_setupdatagram(NULL, EC_CMD_FPWR, idx, ec_slave[slave].configadr, 0x990, 8, &DC_starttime_0);
   uint64 null8 = 0;
   ec_adddatagram(NULL, EC_CMD_FPRD, idx, FALSE, ec_slave[slave].configadr, 0x910, 8, &null8);
   ec_srconfirm(idx, EC_TIMEOUTRET);
   ec_setbufstat(idx, EC_BUF_EMPTY);

   // Set DC Cycle Unit and Activation
   uint8 DC_CU_ACT[2] = {0, 0x7};
   ec_FPWR(ec_slave[slave].configadr, 0x980, 2, &DC_CU_ACT, EC_TIMEOUTRET);

   // Set DC Latch0 and Latch1
   uint8 DC_Latch_01[2] = {0, 0};
   ec_FPWR(ec_slave[slave].configadr, 0x9a0, 2, &DC_Latch_01, EC_TIMEOUTRET);

   // TODO: do 0x600 and 0x610

   // CHECK: If this are all startup commands (see .xml)

   printf("Servo slave %d set, retval = %d (should be 5)\n", slave, retval);
   return retval;
}

void * loop_message(void* ptr) {
   if(!ptr) {
      printf("Error: loop_message should have arguments\n");
      pthread_exit(NULL);
   }

   int chk = *(int*)ptr;

   // Start precise timer for 2000us
   ec_timet current;
   ec_timet prev = osal_current_time();

   while(chk > 0) {
      // Start precise timer for 2000us
      do {
         current = osal_current_time();
      } while((current.sec - prev.sec) * 1000000 + (current.usec - prev.usec) < 2000);
      prev = current;
      ec_alstatust slstat;
      slstat.alstatus = 0;
      slstat.alstatuscode = 0;
      ec_BRD(1100, ECT_REG_ALSTAT, sizeof(slstat), &slstat, EC_TIMEOUTRET);
      ec_5cmds_nop(EC_TIMEOUTRET3);

      // Increment chk
      chk--;
   }

   pthread_exit(NULL);
}

void simpleloop(char *ifname)
{
   int i, j, oloop, iloop, chk;
   needlf = FALSE;
   inOP = FALSE;

   printf("Starting simple test\n");

   /* initialise SOEM, bind socket to ifname */
   if (ec_init(ifname))
   {
      printf("ec_init on %s succeeded.\n",ifname);
      /* find and auto-config slaves */

       if ( ec_config_init(FALSE) > 0 )
      {
         printf("%d slaves found and configured.\n",ec_slavecount);
         // Set state to PREOP
         ec_FPWRw(ec_slave[1].configadr, ECT_REG_ALCTL, htoes(EC_STATE_PRE_OP | EC_STATE_ACK), EC_TIMEOUTRET3);

         // Should be: "Name: AMI8000-0000 ConfiguredAddress: 0x1001 State: 1"
         printf("Name: %s ConfiguredAddress: %#x State: %d Error: %s\n",
               ec_slave[1].name,
               ec_slave[1].configadr,
               ec_slave[1].state,
               ec_ALstatuscode2string(ec_slave[1].ALstatuscode));

         // Set specific config
         ec_slave[1].PO2SOconfig = servo_setup;

         // Should be: "Name: AMI8000-0000 ConfiguredAddress: 0x1001 State: 1"
         printf("Name: %s ConfiguredAddress: %#x State: %d Error: %s\n",
               ec_slave[1].name,
               ec_slave[1].configadr,
               ec_slave[1].state,
               ec_ALstatuscode2string(ec_slave[1].ALstatuscode));

         if (forceByteAlignment)
         {
            ec_config_map_aligned(&IOmap);
         }
         else
         {
            ec_config_map(&IOmap);
         }

         printf("Name: %s ConfiguredAddress: %#x State: %d Error: %s\n",
               ec_slave[1].name,
               ec_slave[1].configadr,
               ec_slave[1].state,
               ec_ALstatuscode2string(ec_slave[1].ALstatuscode));

         ec_configdc();

         // Send 5 cmds with nop
         int iter = 0;
         while(++iter < 4) ec_5cmds_nop(EC_TIMEOUTRET3);

         // Set state to SAFEOP
         ec_slave[1].state = EC_STATE_SAFE_OP;
         ec_writestate(1);
         /* wait for all slaves to reach SAFE_OP state */
         uint16 act_state = ec_so_statecheck(1, EC_STATE_SAFE_OP,  EC_TIMEOUTSTATE);

         // Send 5 cmds with lrw
         iter = 0;
         while(++iter < 100) ec_5cmds_lrw(EC_TIMEOUTRET3);

         printf("Slaves mapped, state to SAFE_OP.\n");
         printf("%d =?= %d\n", EC_STATE_SAFE_OP, act_state);
         printf("Name: %s ConfiguredAddress: %#x State: %d Error: %s\n",
               ec_slave[1].name,
               ec_slave[1].configadr,
               ec_slave[1].state,
               ec_ALstatuscode2string(ec_slave[1].ALstatuscode));

         oloop = ec_slave[0].Obytes;
         if ((oloop == 0) && (ec_slave[0].Obits > 0)) oloop = 1;
         if (oloop > 8) oloop = 8;
         iloop = ec_slave[0].Ibytes;
         if ((iloop == 0) && (ec_slave[0].Ibits > 0)) iloop = 1;
         if (iloop > 8) iloop = 8;

         printf("segments : %d : %d %d %d %d\n",ec_group[0].nsegments ,ec_group[0].IOsegment[0],ec_group[0].IOsegment[1],ec_group[0].IOsegment[2],ec_group[0].IOsegment[3]);

         printf("Request operational state for all slaves\n");
         expectedWKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
         printf("Calculated workcounter %d\n", expectedWKC);
         ec_slave[0].state = EC_STATE_OPERATIONAL;
         /* request OP state for all slaves */
         ec_writestate(0);
         
         // Set attributes for thread to be locked to specific cpu with high (realtime) priority
         pthread_t thread;
         pthread_attr_t attr;
         cpu_set_t cpuset;
         pthread_attr_init(&attr);
         CPU_ZERO(&cpuset);
         CPU_SET(1, &cpuset);
         pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
         pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
         struct sched_param param;
         param.sched_priority = 99;
         pthread_attr_setschedparam(&attr, &param);

         chk = 200;
         // Create thread
         pthread_create(&thread, &attr, loop_message, &chk);

         // Wait for thread to finish
         pthread_join(thread, NULL);

         printf("%d =?= %d\n", EC_STATE_OPERATIONAL, act_state);
         printf("Name: %s ConfiguredAddress: %#x State: %d Error: %s\n",
                ec_slave[1].name,
                ec_slave[1].configadr,
                ec_slave[1].state,
                ec_ALstatuscode2string(ec_slave[1].ALstatuscode));

         if (ec_slave[0].state == EC_STATE_OPERATIONAL )
         {
            printf("Operational state reached for all slaves.\n");

            inOP = TRUE;
                /* cyclic loop */
            for(i = 1; i <= 10000; i++)
            {
               ec_send_processdata();
               wkc = ec_receive_processdata(EC_TIMEOUTRET);

                    if(wkc >= expectedWKC)
                    {
                        printf("Processdata cycle %4d, WKC %d , O:", i, wkc);

                        for(j = 0 ; j < oloop; j++)
                        {
                            printf(" %2.2x", *(ec_slave[0].outputs + j));
                        }

                        printf(" I:");
                        for(j = 0 ; j < iloop; j++)
                        {
                            printf(" %2.2x", *(ec_slave[0].inputs + j));
                        }
                        printf(" T:%"PRId64"\r",ec_DCtime);
                        needlf = TRUE;
                    }
                    osal_usleep(5000);

                }
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
            printf("\nRequest init state for all slaves\n");
            ec_slave[0].state = EC_STATE_INIT;
            /* request INIT state for all slaves */
            ec_writestate(0);
        }
        else
        {
            printf("No slaves found!\n");
        }
        printf("End simple test, close socket\n");
        /* stop SOEM, close socket */
        ec_close();
    }
    else
    {
        printf("No socket connection on %s\nExecute as root\n",ifname);
    }
}

OSAL_THREAD_FUNC ecatcheck( void *ptr )
{
    int slave;
    (void)ptr;                  /* Not used */

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
            // if(!ec_group[currentgroup].docheckstate)
            //    printf("OK : all slaves resumed OPERATIONAL.\n");
        }
        osal_usleep(10000);
    }
}

int main(int argc, char *argv[])
{
   printf("SOEM (Simple Open EtherCAT Master)\nSimple test\n");

   if (argc > 1)
   {
      /* create thread to handle slave error handling in OP */
      osal_thread_create(&thread1, 128000, &ecatcheck, NULL);
      /* start cyclic part */
      simpleloop(argv[1]);
   }
   else
   {
      ec_adaptert * adapter = NULL;
      printf("Usage: simple_test ifname1\nifname = eth0 for example\n");

      printf ("\nAvailable adapters:\n");
      adapter = ec_find_adapters ();
      while (adapter != NULL)
      {
         printf ("    - %s  (%s)\n", adapter->name, adapter->desc);
         adapter = adapter->next;
      }
      ec_free_adapters(adapter);
   }

   printf("End program\n");
   return (0);
}
