#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>

#include "ethercat.h"

void setup() {
    int retval = 0;
        
    uint8_t data2_0[1] = {0x00};
    retval = ec_BWR(0x0000, 0x0101, 1, data2_0, EC_TIMEOUTSAFE);
    printf("BWR 0x0101 TX: 0x00 \n ");
    assert(retval == 1);
                
    uint8_t data4_0[1] = {0x00};
    retval = ec_BWR(0x0000, 0x0101, 1, data4_0, EC_TIMEOUTSAFE);
    printf("BWR 0x0101 TX: 0x00 \n ");
    assert(retval == 1);
                
    uint8_t data6_0[2];
    uint8_t target_data6_0[2] = {0x00, 0x01};
    retval = ec_BRD(0x0000, 0x0130, 2, data6_0, EC_TIMEOUTSAFE);
    printf("BRD 0x0130 TX: 0x0000 RX?: 0x0001 \n ");
    assert(retval == 1);;
    for (int i = 0; i < 2; i++) {
      assert(data6_0[i] == target_data6_0[i]);
    }
                
    uint8_t data9_0[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    retval = ec_BWR(0x0000, 0x0300, 8, data9_0, EC_TIMEOUTSAFE);
    printf("BWR 0x0300 TX: 0x0000000000000000 \n ");
    assert(retval == 1);
                
    uint8_t data9_1[2];
    uint8_t target_data9_1[2] = {0x00, 0x01};
    retval = ec_BRD(0x0000, 0x0130, 2, data9_1, EC_TIMEOUTSAFE);
    printf("BRD 0x0130 TX: 0x0000 RX?: 0x0001 \n ");
    assert(retval == 1);;
    for (int i = 0; i < 2; i++) {
      assert(data9_1[i] == target_data9_1[i]);
    }
                
    uint8_t data10_0[10];
    uint8_t target_data10_0[10] = {0x01, 0xcc, 0x1f, 0x08, 0x08, 0x08, 0x00, 0x00, 0x03, 0xc0};
    retval = ec_APRD(0x0000, 0x0000, 10, data10_0, EC_TIMEOUTSAFE);
    printf("APRD 0x0000 TX: 0x00000000000000000000 RX?: 0x01cc1f080808000003c0 \n ");
    assert(retval == 1);;
    for (int i = 0; i < 10; i++) {
      assert(data10_0[i] == target_data10_0[i]);
    }
                
    uint8_t data10_1[1];
    uint8_t target_data10_1[1] = {0x07};
    retval = ec_APRD(0x0000, 0x0102, 1, data10_1, EC_TIMEOUTSAFE);
    printf("APRD 0x0102 TX: 0x00 RX?: 0x07 \n ");
    assert(retval == 1);;
    for (int i = 0; i < 1; i++) {
      assert(data10_1[i] == target_data10_1[i]);
    }
                
    uint8_t data10_2[1];
    uint8_t target_data10_2[1] = {0x56};
    retval = ec_APRD(0x0000, 0x0111, 1, data10_2, EC_TIMEOUTSAFE);
    printf("APRD 0x0111 TX: 0x00 RX?: 0x56 \n ");
    assert(retval == 1);;
    for (int i = 0; i < 1; i++) {
      assert(data10_2[i] == target_data10_2[i]);
    }
        
}

int main(int argc, char *argv[])
{

   if (argc < 2) {
      ec_adaptert * adapter = NULL;
      printf("Usage: replay ifname1\nifname = eth0 for example\n");

      printf ("\nAvailable adapters:\n");
      adapter = ec_find_adapters ();
      while (adapter != NULL)
      {
         printf ("    - %s  (%s)\n", adapter->name, adapter->desc);
         adapter = adapter->next;
      }
      ec_free_adapters(adapter);

      return -1;
   }

   char *ifname = argv[1];

   // Init socket
   if (!ec_init(ifname))
   {
      printf("No socket connection on %s\nExecute as root\n", ifname);
      return -1;
   }

   setup();

   ec_close();
   return 0;
}
