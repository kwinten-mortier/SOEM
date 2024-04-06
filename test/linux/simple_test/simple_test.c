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
#define STATUS_WORD_MASK(x) (x &= 0x6F)
#define MODES_OF_OPERATION_INDEX (0x6060)

/** Struct for specific outputs **/
// Outputs (6 bytes (pad to 10)): Controlword (2 bytes) + Target position (4 bytes) + padding (4 bytes)
typedef struct
{
   uint16 controlword;
   uint32 target_pos;
} __attribute__((packed)) our_outputs;

/** Struct for specific inputs **/
// Inputs (10 bytes): postion (4 bytes) + statusword (2 bytes) + error actual value (4 bytes)
typedef struct
{
   uint32 position;
   uint16 statusword;
   int32 erroract;
} __attribute__((packed)) our_inputs;

typedef enum
{
   control_switch_on = 0,
   control_enable_voltage = 1,
   control_quick_stop = 2,
   control_enable_operation = 3,
   control_fault_reset = 7,
   control_4 = 4,
   control_5 = 5,
   control_6 = 6,
   control_8 = 8
} control_bit_t;

typedef enum
{
   Not_ready_to_switch_on = 0x0,
   Switch_on_disabled = 1 << 6,
   Ready_to_switch_on = (1 << 5) | 1,
   Switch_on = (1 << 5) | 3,
   Operation_enabled = (1 << 5) | 7,
   Quick_stop_active = 0x7,
   Fault_reaction_active = 0x0f,
   Fault = 1 << 3
} statusword_state_t;

uint16_t controlword = 0x0;

char IOmap[4096];
OSAL_THREAD_HANDLE thread1;
int expectedWKC;
boolean needlf;
volatile int wkc;
boolean inOP;
uint8 currentgroup = 0;
boolean forceByteAlignment = FALSE;

void set_output_int16(uint16_t slave_nb, uint16_t controlword, uint32_t value)
{
   our_outputs *data_ptr;

   data_ptr = (our_outputs *)ec_slave[slave_nb].outputs;
   data_ptr->controlword = controlword;
   data_ptr->target_pos = value;
}

// Run all startup/init commands (for PREOP->SAFEOP), should return 1
int configservo(uint16 slave)
{
   uint8 u8val;
   uint16 u16val;
   uint32 u32val;
   int retval = 0;

   // Motor Settings

   // Set max current
   u32val = 5000;
   retval += ec_SDOwrite(slave, 0x8011, 0x11, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTSAFE);
   printf("0x8011:11: retval = %d\n", retval);

   // Set rated current
   u32val = 1000;
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
   u32val = 14000;
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

   // 0x8010:17
   u32val = 1;
   retval += ec_SDOwrite(slave, 0x8010, 0x17, FALSE, sizeof(u32val), &u32val, EC_TIMEOUTSAFE);
   printf("0x8010:17: retval = %d\n", retval);

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

our_inputs get_input_int16(uint16_t slave_nb)
{
   our_inputs *inputs = (our_inputs *)ec_slave[slave_nb].inputs;

   return *inputs;
}

void *loop_message(void *ptr)
{
   if (!ptr)
   {
      printf("Error: loop_message should have arguments\n");
      pthread_exit(NULL);
   }

   int chk = *(int *)ptr;

   int i, j, oloop, iloop;

   oloop = ec_slave[0].Obytes;
   if ((oloop == 0) && (ec_slave[0].Obits > 0))
      oloop = 1;
   if (oloop > 8)
      oloop = 8;
   iloop = ec_slave[0].Ibytes;
   if ((iloop == 0) && (ec_slave[0].Ibits > 0))
      iloop = 1;
   if (iloop > 8)
      iloop = 8;

   // Start precise timer for 2000us
   // use clock_gettime(CLOCK_MONOTONIC, &ts) for better precision
   struct timespec tcur = {0, 0}, tprev = {0, 0};
   clock_gettime(CLOCK_MONOTONIC, &tcur);
   tprev = tcur;

   uint32_t target;
   our_inputs inputs;

   ec_send_processdata();
   wkc = ec_receive_processdata(EC_TIMEOUTRET);

   inputs = get_input_int16(1);

   int operation_enabled = 0;
   uint32_t relative_offset = 0;
   int direction = 1;

   target = inputs.position;

   while (chk > 0)
   {
      i = chk;
      // Start precise timer for 2000us
      do
      {
         clock_gettime(CLOCK_MONOTONIC, &tcur);
      } while ((tcur.tv_sec - tprev.tv_sec) * 1000000000 + (tcur.tv_nsec - tprev.tv_nsec) < 2000000);
      tprev = tcur;

      // Do logic
      ec_send_processdata();
      wkc = ec_receive_processdata(EC_TIMEOUTRET);
      inputs = get_input_int16(1);

      if (wkc >= expectedWKC)
      {

         needlf = TRUE;

         int16_t statusword = inputs.statusword;
         STATUS_WORD_MASK(statusword);
         if (operation_enabled == 0)
         {
            switch (statusword)
            {
            case (Not_ready_to_switch_on):
            {
               /* Now the FSM should automatically go to Switch_on_disabled*/
               break;
            }
            case (Switch_on_disabled):
            case (Switch_on_disabled | (0b1 << 5)):
            {
               /* Automatic transition (2)*/
               controlword = 0;
               controlword |= (1 << control_enable_voltage) | (1 << control_quick_stop);
               if (i % 10 == 0)
               {
                  controlword |= 1 << control_fault_reset;
               }
               break;
            }
            case (Ready_to_switch_on):
               // case (Ready_to_switch_on | (0b1 << 5)):
               {
                  /* Switch on command for transition (3) */
                  controlword |= 1 << control_switch_on;
                  controlword = 0x07;
                  break;
               }
            case (Switch_on):
            {
               /* Enable operation command for transition (4) */
               controlword |= 1 << control_enable_operation;
               break;
            }
            case (Operation_enabled):
            {
               /* Setting modes of operation
                        * Value Description
                        -128...-2 Reserved
                        -1 No mode
                        0 Reserved
                        1 Profile position mode
                        2 Velocity (not supported)
                        3 Profiled velocity mode
                        4 Torque profiled mode
                        5 Reserved
                        6 Homing mode
                        7 Interpolated position mode
                        8 Cyclic Synchronous position
                        ...127 Reserved*/

               uint16_t mode = 8; /* Setting Cyclic Synchronous position */
               int mode_size = sizeof(mode);
               int SDO_result = ec_SDOwrite(1, MODES_OF_OPERATION_INDEX, 0,
                                            0, mode_size, &mode, EC_TIMEOUTRXM);
               controlword |= 0x1f;

               operation_enabled = 1;
               break;
            }
            case (Quick_stop_active):
            {
               break;
            }
            case (Fault_reaction_active):
            {
               break;
            }
            case (Fault):
            case (0x28):
            {
               /* Returning to Switch on Disabled */
               controlword = (1 << control_fault_reset);
               controlword |= (1 << control_enable_voltage) | (1 << control_quick_stop);
               break;
            }
            default:
            {
               printf("Unrecognized status\n");
               break;
            }
            }
         }

         relative_offset += 500 * direction;

         if (direction == 1 && (int32_t )target - (int32_t) inputs.position > 250000)
         {
            relative_offset = 250000; //target - inputs.position;
         } 
         else if (direction == -1 && (int32_t)inputs.position - (int32_t)target > 250000)
         {
            relative_offset = -250000; //target - inputs.position;
         }

         target = inputs.position + relative_offset;

         printf("Target: %d ", target);
         printf("Value: %d ", inputs.position);
         printf("Relative offset: %d ", relative_offset);
         printf("Controlword: %#x ", controlword);
         printf("Statusword: %#x (%d) ", statusword, statusword);
         printf("Error: %d\r", inputs.erroract);

         set_output_int16(1, controlword, target);
      }

      // Increment chk
      chk--;

      if (chk % 2000 == 0) {
         direction = -direction;
      }
   }

   pthread_exit(NULL);
}

void start_loop(int chk)
{
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

   // Create thread
   pthread_create(&thread, &attr, loop_message, &chk);

   // Wait for thread to finish
   pthread_join(thread, NULL);
}

void simpletest(char *ifname)
{
   int i, j, oloop, iloop, chk;
   needlf = FALSE;
   inOP = FALSE;

   printf("Starting simple test\n");

   /* initialise SOEM, bind socket to ifname */
   if (ec_init(ifname))
   {
      printf("ec_init on %s succeeded.\n", ifname);
      /* find and auto-config slaves */

      if (ec_config_init(FALSE) > 0)
      {

         printf("%d slaves found and configured.\n", ec_slavecount);

         ec_slave[1].PO2SOconfig = configservo;

         if (forceByteAlignment)
         {
            ec_config_map_aligned(&IOmap);
         }
         else
         {
            ec_config_map(&IOmap);
         }

         ec_configdc();

         printf("Slaves mapped, state to SAFE_OP.\n");
         /* wait for all slaves to reach SAFE_OP state */
         ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

         osal_usleep(100000);

         oloop = ec_slave[0].Obytes;
         if ((oloop == 0) && (ec_slave[0].Obits > 0))
            oloop = 1;
         if (oloop > 8)
            oloop = 8;
         iloop = ec_slave[0].Ibytes;
         if ((iloop == 0) && (ec_slave[0].Ibits > 0))
            iloop = 1;
         if (iloop > 8)
            iloop = 8;

         printf("segments : %d : %d %d %d %d\n", ec_group[0].nsegments, ec_group[0].IOsegment[0], ec_group[0].IOsegment[1], ec_group[0].IOsegment[2], ec_group[0].IOsegment[3]);

         printf("Request operational state for all slaves\n");
         expectedWKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
         printf("Calculated workcounter %d\n", expectedWKC);
         ec_slave[0].state = EC_STATE_OPERATIONAL;
         /* send one valid process data to make outputs in slaves happy*/
         // ec_send_processdata();
         // ec_receive_processdata(EC_TIMEOUTRET);
         /* request OP state for all slaves */
         ec_writestate(0);
         chk = 200;
         /* wait for all slaves to reach OP state */
         do
         {
            ec_send_processdata();
            // ec_writestate(0);
            ec_receive_processdata(EC_TIMEOUTRET);
            ec_statecheck(0, EC_STATE_OPERATIONAL, 50000);
         } while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));

         ec_statecheck(0, EC_STATE_OPERATIONAL, EC_TIMEOUTSTATE * 4);

         osal_usleep(1000000);

         if (ec_slave[0].state == EC_STATE_OPERATIONAL)
         {
            printf("Operational state reached for all slaves.\n");
            inOP = TRUE;

            ec_send_processdata();
            wkc = ec_receive_processdata(EC_TIMEOUTRET);

            printf("WKC: %d\n", wkc);

            if (wkc >= 1)
            {

               // Start precise loop
               start_loop(20000);
            }

            /* cyclic loop */
            // for (i = 1; i <= 10000; i++)
            // {
            //    ec_send_processdata();
            //    wkc = ec_receive_processdata(EC_TIMEOUTRET);

            //    if (wkc >= expectedWKC)
            //    {
            //       printf("Processdata cycle %4d, WKC %d , O:", i, wkc);

            //       for(j = 0 ; j < oloop; j++)
            //       {
            //           printf(" %2.2x", *(ec_slave[0].outputs + j));
            //       }

            //       printf(" I:");
            //       for(j = 0 ; j < iloop; j++)
            //       {
            //           printf(" %2.2x", *(ec_slave[0].inputs + j));
            //       }
            //       printf(" T:%"PRId64"\r",ec_DCtime);
            //       needlf = TRUE;

            //       int16_t statusword = 0;
            //       /* The input address for get_input_int16 must be devided by two. 0xA
            //       is the starting address of inputs*/
            //       get_input_int16(1, (0xC - 0x8) >> 1, (int16_t *)&statusword);

            //       // statusword = statusword >> 1;
            //       // statusword = statusword & ~(0b1 << 5);

            //       STATUS_WORD_MASK(statusword);
            //       printf("Statusword: %#x (%d)\n", statusword, statusword);

            //       switch (statusword)
            //       {
            //       case (Not_ready_to_switch_on):
            //       {
            //          /* Now the FSM should automatically go to Switch_on_disabled*/
            //          break;
            //       }
            //       case (Switch_on_disabled):
            //       case (Switch_on_disabled | (0b1 << 5)):
            //       {
            //          /* Automatic transition (2)*/
            //          controlword = 0;
            //          controlword |= (1 << control_enable_voltage) | (1 << control_quick_stop);
            //          if(i % 10 == 0) {
            //             controlword |= 1 << control_fault_reset;
            //          }
            //          break;
            //       }
            //       case (Ready_to_switch_on):
            //       // case (Ready_to_switch_on | (0b1 << 5)):
            //       {
            //          /* Switch on command for transition (3) */
            //          controlword |= 1 << control_switch_on;
            //          controlword = 0x01;
            //          break;
            //       }
            //       case (Switch_on):
            //       {
            //          /* Enable operation command for transition (4) */
            //          controlword |= 1 << control_enable_operation;
            //          break;
            //       }
            //       case (Operation_enabled):
            //       {
            //          /* Setting modes of operation
            //                  * Value Description
            //                  -128...-2 Reserved
            //                  -1 No mode
            //                  0 Reserved
            //                  1 Profile position mode
            //                  2 Velocity (not supported)
            //                  3 Profiled velocity mode
            //                  4 Torque profiled mode
            //                  5 Reserved
            //                  6 Homing mode
            //                  7 Interpolated position mode
            //                  8 Cyclic Synchronous position
            //                  ...127 Reserved*/

            //          uint16_t mode = 8; /* Setting Cyclic Synchronous position */
            //          int mode_size = sizeof(mode);
            //          int SDO_result = ec_SDOwrite(1, MODES_OF_OPERATION_INDEX, 0,
            //                                   0, mode_size, &mode, EC_TIMEOUTRXM);
            //          break;
            //       }
            //       case (Quick_stop_active):
            //       {
            //          break;
            //       }
            //       case (Fault_reaction_active):
            //       {
            //          break;
            //       }
            //       case (Fault):
            //       case (0x28):
            //       {
            //          /* Returning to Switch on Disabled */
            //          controlword = (1 << control_fault_reset);
            //          controlword |= (1 << control_enable_voltage) | (1 << control_quick_stop);
            //          break;
            //       }
            //       default:
            //       {
            //          printf("Unrecognized status\n");
            //          break;
            //       }
            //       }

            //       // printf("Controlword: %#x\n", controlword);
            //       set_output_int16(1, 0 >> 1, controlword);

            //       int16_t value_high = 0;
            //       int16_t value_low = 0;
            //       int32_t value = 0;

            //       get_input_int16(1, (0x6 - 0x6) >> 1, (int16_t*) &value_low);
            //       get_input_int16(1, (0x8 - 0x6) >> 1, (int16_t*) &value_high);
            //       value = ((value_high << 16) & 0xffff0000) | (value_low & 0x00ffff);
            //       //  value += 3;

            //       // value = 0x07070707;

            //       /* Setting output */
            //       set_output_int16(1, 0x2 >> 1, (int16_t)(value & 0xffff) );
            //       set_output_int16(1, 0x4 >> 1, (int16_t)((value >> 16) & 0xffff));

            //    }
            //    osal_usleep(1000);
            // }
            inOP = FALSE;
         }
         else
         {
            printf("Not all slaves reached operational state.\n");
            ec_readstate();
            for (i = 1; i <= ec_slavecount; i++)
            {
               if (ec_slave[i].state != EC_STATE_OPERATIONAL)
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
      printf("No socket connection on %s\nExecute as root\n", ifname);
   }
}

OSAL_THREAD_FUNC ecatcheck(void *ptr)
{
   int slave;
   (void)ptr; /* Not used */

   while (1)
   {
      if (inOP && ((wkc < expectedWKC) || ec_group[currentgroup].docheckstate))
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
               else if (ec_slave[slave].state == EC_STATE_SAFE_OP)
               {
                  printf("WARNING : slave %d is in SAFE_OP, change to OPERATIONAL.\n", slave);
                  ec_slave[slave].state = EC_STATE_OPERATIONAL;
                  ec_writestate(slave);
               }
               else if (ec_slave[slave].state > EC_STATE_NONE)
               {
                  if (ec_reconfig_slave(slave, EC_TIMEOUTMON))
                  {
                     ec_slave[slave].islost = FALSE;
                     printf("MESSAGE : slave %d reconfigured\n", slave);
                  }
               }
               else if (!ec_slave[slave].islost)
               {
                  /* re-check state */
                  ec_statecheck(slave, EC_STATE_OPERATIONAL, EC_TIMEOUTRET);
                  if (ec_slave[slave].state == EC_STATE_NONE)
                  {
                     ec_slave[slave].islost = TRUE;
                     printf("ERROR : slave %d lost\n", slave);
                  }
               }
            }
            if (ec_slave[slave].islost)
            {
               if (ec_slave[slave].state == EC_STATE_NONE)
               {
                  if (ec_recover_slave(slave, EC_TIMEOUTMON))
                  {
                     ec_slave[slave].islost = FALSE;
                     printf("MESSAGE : slave %d recovered\n", slave);
                  }
               }
               else
               {
                  ec_slave[slave].islost = FALSE;
                  printf("MESSAGE : slave %d found\n", slave);
               }
            }
         }
         if (!ec_group[currentgroup].docheckstate)
            printf("OK : all slaves resumed OPERATIONAL.\n");
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
      simpletest(argv[1]);
   }
   else
   {
      ec_adaptert *adapter = NULL;
      printf("Usage: simple_test ifname1\nifname = eth0 for example\n");

      printf("\nAvailable adapters:\n");
      adapter = ec_find_adapters();
      while (adapter != NULL)
      {
         printf("    - %s  (%s)\n", adapter->name, adapter->desc);
         adapter = adapter->next;
      }
      ec_free_adapters(adapter);
   }

   printf("End program\n");
   return (0);
}
