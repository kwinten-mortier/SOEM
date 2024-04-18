#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>

#include "ethercat.h"

void printHexCharArray(const uint8_t *array, size_t length)
{
   for (size_t i = length; i > 0; i--)
   {
      printf("%02x", (unsigned char)array[i - 1]);
   }
}

void transition_IP()
{
   int retval = 0;

   int success = 0;
   uint8_t data[2048];

   /**
    * Go to INIT
    */
   retval = ec_APWR(0x0, 0x120, 2, (uint8_t[]){0x11, 0x00}, EC_TIMEOUTRXM);
   assert(retval == 1);
   printf("1: APWR 0x120 TX: 0x0011 \n");
   success = 0;
   for (int i = 0; i < 1000; i++)
   {
      retval = ec_APRD(0x0, 0x130, 2, data, EC_TIMEOUTRXM);
      assert(retval == 1);

      if (memcmp(data, (uint8_t[]){0x01, 0x00}, 2) == 0)
      {
         success = 1;
         break;
      }

      osal_usleep(1000);
   }
   if (!success)
   {
      printf("    Received: 0x");
      printHexCharArray(data, 2);
      printf("\n");
      assert(0);
   }

   /**
    * Clear FFMUs
    */
   printf("Clear FMMUs\n");
   memset(data, 0, sizeof data);
   retval = ec_BWR(0x3e9, 0x600, 256, data, EC_TIMEOUTRXM);
   assert(retval == 1);

   /**
    * Clear SyncManagers
    */
   printf("Clear SyncManagers\n");
   memset(data, 0, sizeof data);
   retval = ec_BWR(0x3e9, 0x800, 128, data, EC_TIMEOUTRXM);
   assert(retval == 1);

   /**
    * Set fixed physical address
    */
   printf("Set fixed physical address\n");
   retval = ec_APWR(0x0, 0x10, 2, (uint8_t[]){0x34, 0x12}, EC_TIMEOUTRXM);
   assert(retval == 1);

   /**
    * Set SM0 and SM1
    */
   printf("Set SM0 and SM1\n");
   retval = ec_FPWR(0x1234, 0x800, 8, (uint8_t[]){0x00, 0x10, 0x00, 0x01, 0x26, 0x00, 0x01, 0x00}, EC_TIMEOUTRXM);
   assert(retval == 1);
   printf("Set SM0 and SM1\n");
   retval = ec_FPWR(0x1234, 0x808, 8, (uint8_t[]){0x00, 0x11, 0x00, 0x01, 0x22, 0x00, 0x01, 0x00}, EC_TIMEOUTRXM);
   assert(retval == 1);

   // Mailbox should be available now!
   /**
    * Go to PREOP
    */
   printf("Go to PREOP\n");
   retval = ec_APWR(0x0, 0x120, 2, (uint8_t[]){0x12, 0x00}, EC_TIMEOUTRXM);
   assert(retval == 1);
   success = 0;
   for (int i = 0; i < 1000; i++)
   {
      retval = ec_APRD(0x0, 0x130, 2, data, EC_TIMEOUTRXM);
      assert(retval == 1);

      if (memcmp(data, (uint8_t[]){0x02, 0x00}, 2) == 0)
      {
         success = 1;
         break;
      }

      osal_usleep(1000);
   }
   if (!success)
   {
      printf("    Received: 0x");
      printHexCharArray(data, 2);
      printf("\n");
      assert(0);
   }
}

void transition_PS()
{
   int retval = 0;

   /**
    * Initiate download
    */
   printf("SDO Download f081\n");
   uint8_t data573_5[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x20, 0x23, 0x81, 0xf0, 0x01, 0x00, 0x00, 0x11, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_5, EC_TIMEOUTRXM);
   assert(retval == 1);
   osal_usleep(50000);
   uint8_t data573_6[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_6, EC_TIMEOUTRXM);
   assert(retval == 1);
   osal_usleep(50000);
   uint8_t data573_7[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_7, EC_TIMEOUTRXM);
   assert(retval == 1);
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_7, 256);
   printf("\n");

   osal_usleep(50000);

   /**
    * Initiate download  10f3
    */
   printf("SDO Download 10f3\n");
   uint8_t data573_8[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x20, 0x2b, 0xf3, 0x10, 0x05, 0x01, 0x00, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_8, EC_TIMEOUTRXM);
   assert(retval == 1);
   osal_usleep(50000);
   uint8_t data573_9[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_9, EC_TIMEOUTRXM);
   assert(retval == 1);
   osal_usleep(50000);
   uint8_t data573_10[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_10, EC_TIMEOUTRXM);
   assert(retval == 1);
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_10, 256);
   printf("\n");

   osal_usleep(50000);

   /**
    * Initiate download 1c12
    */
   printf("SDO Download 1c12\n");
   uint8_t data573_45[22] = {0x10, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x20, 0x31, 0x12, 0x1c, 0x00, 0x06, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x16, 0x06, 0x16};
   retval = ec_FPWR(0x1234, 0x1000, 22, data573_45, EC_TIMEOUTRXM);
   assert(retval == 1);
   osal_usleep(50000);
   uint8_t data573_46[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_46, EC_TIMEOUTRXM);
   assert(retval == 1);
   osal_usleep(50000);
   uint8_t data573_47[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_47, EC_TIMEOUTRXM);
   assert(retval == 1);
   osal_usleep(50000);

   printf("    Received: 0x");
   printHexCharArray(data573_47, 256);
   printf("\n");

   osal_usleep(50000);

   /**
    * Initiate download 1c13
    */
   printf("SDO Download 1c13\n");
   uint8_t data573_48[24] = {0x12, 0x00, 0x00, 0x00, 0x00, 0x33, 0x00, 0x20, 0x31, 0x13, 0x1c, 0x00, 0x08, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x1a, 0x01, 0x1a, 0x06, 0x1a};
   retval = ec_FPWR(0x1234, 0x1000, 24, data573_48, EC_TIMEOUTRXM);
   assert(retval == 1);
   osal_usleep(50000);
   uint8_t data573_49[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_49, EC_TIMEOUTRXM);
   assert(retval == 1);
   osal_usleep(50000);
   uint8_t data573_50[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_50, EC_TIMEOUTRXM);
   assert(retval == 1);
   osal_usleep(50000);

   printf("    Received: 0x");
   printHexCharArray(data573_50, 256);
   printf("\n");

   osal_usleep(50000);

   // scanf("%c",&ch);
   uint8_t data573_51[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x43, 0x00, 0x20, 0x23, 0x11, 0x80, 0x11, 0x80, 0xbb, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_51, EC_TIMEOUTRXM);
   printf("102: FPWR 0x1000 TX: 0x0000bb8011801123200043000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_52[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_52, EC_TIMEOUTRXM);
   printf("104: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_53[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_53, EC_TIMEOUTRXM);
   printf("106: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x00 \n");
   assert(retval == 1);
   ;
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_53, 256);
   printf("\n");

   // scanf("%c",&ch);
   uint8_t data573_54[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x53, 0x00, 0x20, 0x23, 0x11, 0x80, 0x12, 0x40, 0x1f, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_54, EC_TIMEOUTRXM);
   printf("108: FPWR 0x1000 TX: 0x00001f4012801123200053000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_55[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_55, EC_TIMEOUTRXM);
   printf("110: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_56[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_56, EC_TIMEOUTRXM);
   printf("112: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x00 \n");
   assert(retval == 1);
   ;
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_56, 256);
   printf("\n");

   osal_usleep(50000);

   // scanf("%c",&ch);
   uint8_t data573_57[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x20, 0x2f, 0x11, 0x80, 0x13, 0x03, 0x00, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_57, EC_TIMEOUTRXM);
   printf("114: FPWR 0x1000 TX: 0x000000031380112f200063000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_58[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_58, EC_TIMEOUTRXM);
   printf("116: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_59[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_59, EC_TIMEOUTRXM);
   printf("118: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x00 \n");
   assert(retval == 1);
   ;
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_59, 256);
   printf("\n");
   // HALLO

   // scanf("%c",&ch);
   uint8_t data573_60[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x73, 0x00, 0x20, 0x23, 0x11, 0x80, 0x16, 0x64, 0x00, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_60, EC_TIMEOUTRXM);
   printf("120: FPWR 0x1000 TX: 0x0000006416801123200073000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_61[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_61, EC_TIMEOUTRXM);
   printf("122: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);

   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_65[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_65, EC_TIMEOUTRXM);
   printf("130: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x000000000002 \n");
   assert(retval == 1);
   ;
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_65, 256);
   printf("\n");

   uint8_t data573_66[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x20, 0x23, 0x11, 0x80, 0x18, 0xfd, 0x00, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_66, EC_TIMEOUTRXM);
   printf("132: FPWR 0x1000 TX: 0x000000fd18801123200013000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_67[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_67, EC_TIMEOUTRXM);
   printf("134: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_68[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_68, EC_TIMEOUTRXM);
   printf("136: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x00 \n");
   assert(retval == 1);
   ;
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_68, 256);
   printf("\n");

   osal_usleep(50000);

   // scanf("%c",&ch);
   uint8_t data573_69[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x20, 0x2b, 0x11, 0x80, 0x19, 0x46, 0x00, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_69, EC_TIMEOUTRXM);
   printf("138: FPWR 0x1000 TX: 0x000000461980112b200023000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_70[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_70, EC_TIMEOUTRXM);
   printf("140: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_71[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_71, EC_TIMEOUTRXM);
   printf("142: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x00 \n");
   assert(retval == 1);
   ;
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_71, 256);
   printf("\n");

   osal_usleep(50000);

   // scanf("%c",&ch);
   uint8_t data573_72[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x33, 0x00, 0x20, 0x23, 0x11, 0x80, 0x1b, 0xe0, 0x2e, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_72, EC_TIMEOUTRXM);
   printf("144: FPWR 0x1000 TX: 0x00002ee01b801123200033000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_73[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_73, EC_TIMEOUTRXM);
   printf("146: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_74[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_74, EC_TIMEOUTRXM);
   printf("148: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x00 \n");
   assert(retval == 1);
   ;
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_74, 256);
   printf("\n");

   osal_usleep(50000);

   // scanf("%c",&ch);
   uint8_t data573_75[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x43, 0x00, 0x20, 0x2b, 0x11, 0x80, 0x2b, 0xb0, 0x04, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_75, EC_TIMEOUTRXM);
   printf("150: FPWR 0x1000 TX: 0x000004b02b80112b200043000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_76[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_76, EC_TIMEOUTRXM);
   printf("152: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_77[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_77, EC_TIMEOUTRXM);
   printf("154: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x00 \n");
   assert(retval == 1);
   ;
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_77, 256);
   printf("\n");

   osal_usleep(50000);

   // scanf("%c",&ch);
   uint8_t data573_78[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x53, 0x00, 0x20, 0x2b, 0x11, 0x80, 0x2c, 0x78, 0x05, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_78, EC_TIMEOUTRXM);
   printf("156: FPWR 0x1000 TX: 0x000005782c80112b200053000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_79[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_79, EC_TIMEOUTRXM);
   printf("158: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_80[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_80, EC_TIMEOUTRXM);
   printf("160: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x00 \n");
   assert(retval == 1);
   ;
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_80, 256);
   printf("\n");

   // scanf("%c",&ch);
   uint8_t data573_81[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x20, 0x2b, 0x11, 0x80, 0x2d, 0x1a, 0x04, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_81, EC_TIMEOUTRXM);
   printf("162: FPWR 0x1000 TX: 0x0000041a2d80112b200063000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_82[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_82, EC_TIMEOUTRXM);
   printf("164: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_83[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_83, EC_TIMEOUTRXM);
   printf("166: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x00 \n");
   assert(retval == 1);
   ;
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_83, 256);
   printf("\n");

   osal_usleep(50000);

   // scanf("%c",&ch);
   uint8_t data573_84[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x73, 0x00, 0x20, 0x23, 0x11, 0x80, 0x30, 0x68, 0x01, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_84, EC_TIMEOUTRXM);
   printf("168: FPWR 0x1000 TX: 0x0000016830801123200073000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_85[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_85, EC_TIMEOUTRXM);
   printf("170: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_86[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_86, EC_TIMEOUTRXM);
   printf("172: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x00 \n");
   assert(retval == 1);
   ;
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_86, 256);
   printf("\n");

   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_87[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x20, 0x23, 0x11, 0x80, 0x31, 0x7c, 0x15, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_87, EC_TIMEOUTRXM);
   printf("174: FPWR 0x1000 TX: 0x0000157c31801123200013000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_88[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_88, EC_TIMEOUTRXM);
   printf("176: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_89[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_89, EC_TIMEOUTRXM);
   printf("178: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x00 \n");
   assert(retval == 1);
   ;
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_89, 256);
   printf("\n");
   osal_usleep(50000);

   // scanf("%c",&ch);
   uint8_t data573_90[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x20, 0x2b, 0x11, 0x80, 0x15, 0x0e, 0x01, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_90, EC_TIMEOUTRXM);
   printf("180: FPWR 0x1000 TX: 0x0000010e1580112b200023000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_91[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_91, EC_TIMEOUTRXM);
   printf("182: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);
   osal_usleep(50000);

   // scanf("%c",&ch);
   uint8_t data573_94[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_94, EC_TIMEOUTRXM);
   printf("188: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x0000000000000000 \n");
   assert(retval == 1);
   ;
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_94, 256);
   printf("\n");

   osal_usleep(50000);

   // scanf("%c",&ch);
   uint8_t data573_95[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x33, 0x00, 0x20, 0x2b, 0x10, 0x70, 0x0b, 0xca, 0x08, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_95, EC_TIMEOUTRXM);
   printf("190: FPWR 0x1000 TX: 0x000008ca0b70102b200033000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_96[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_96, EC_TIMEOUTRXM);
   printf("192: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_97[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_97, EC_TIMEOUTRXM);
   printf("194: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x00 \n");
   assert(retval == 1);
   ;
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_97, 256);
   printf("\n");

   osal_usleep(50000);

   // scanf("%c",&ch);
   uint8_t data573_98[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x43, 0x00, 0x20, 0x2b, 0x10, 0x80, 0x13, 0x27, 0x00, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_98, EC_TIMEOUTRXM);
   printf("196: FPWR 0x1000 TX: 0x000000271380102b200043000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_99[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_99, EC_TIMEOUTRXM);
   printf("198: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_100[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_100, EC_TIMEOUTRXM);
   printf("200: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x00 \n");
   assert(retval == 1);
   ;
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_100, 256);
   printf("\n");

   osal_usleep(50000);

   // scanf("%c",&ch);
   uint8_t data573_101[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x53, 0x00, 0x20, 0x2b, 0x10, 0x80, 0x12, 0x13, 0x00, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_101, EC_TIMEOUTRXM);
   printf("202: FPWR 0x1000 TX: 0x000000131280102b200053000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_102[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_102, EC_TIMEOUTRXM);
   printf("204: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);
   osal_usleep(50000);

   // scanf("%c",&ch);
   uint8_t data573_104[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_104, EC_TIMEOUTRXM);
   printf("208: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x000000000002 \n");
   assert(retval == 1);
   ;
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_104, 256);
   printf("\n");

   osal_usleep(50000);

   // scanf("%c",&ch);
   uint8_t data573_105[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x20, 0x23, 0x10, 0x80, 0x15, 0xe0, 0x00, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_105, EC_TIMEOUTRXM);
   printf("210: FPWR 0x1000 TX: 0x000000e015801023200063000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_106[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_106, EC_TIMEOUTRXM);
   printf("212: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_107[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_107, EC_TIMEOUTRXM);
   printf("214: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x00 \n");
   assert(retval == 1);
   ;

   printf("    Received: 0x");
   printHexCharArray(data573_107, 256);
   printf("\n");

   osal_usleep(50000);

   // scanf("%c",&ch);
   uint8_t data573_108[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x73, 0x00, 0x20, 0x23, 0x10, 0x80, 0x14, 0x96, 0x00, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_108, EC_TIMEOUTRXM);
   printf("216: FPWR 0x1000 TX: 0x0000009614801023200073000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_109[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_109, EC_TIMEOUTRXM);
   printf("218: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_110[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_110, EC_TIMEOUTRXM);
   printf("220: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x00 \n");
   assert(retval == 1);
   ;
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_110, 256);
   printf("\n");

   osal_usleep(50000);

   // scanf("%c",&ch);
   uint8_t data573_111[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x20, 0x2f, 0x00, 0x80, 0x15, 0x64, 0x00, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_111, EC_TIMEOUTRXM);
   printf("222: FPWR 0x1000 TX: 0x000000641580002f200013000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_112[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_112, EC_TIMEOUTRXM);
   printf("224: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_113[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_113, EC_TIMEOUTRXM);
   printf("226: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x00 \n");
   assert(retval == 1);
   ;
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_113, 256);
   printf("\n");

   osal_usleep(50000);

   // scanf("%c",&ch);
   uint8_t data573_114[16] = {0x0a, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x20, 0x23, 0x10, 0x80, 0x19, 0xc0, 0x5d, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x1000, 16, data573_114, EC_TIMEOUTRXM);
   printf("228: FPWR 0x1000 TX: 0x00005dc019801023200023000000000a \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_115[1] = {0x00};
   retval = ec_FPWR(0x1234, 0x10ff, 1, data573_115, EC_TIMEOUTRXM);
   printf("230: FPWR 0x10ff TX: 0x00 \n");
   assert(retval == 1);
   osal_usleep(50000);
   // scanf("%c",&ch);
   uint8_t data573_116[256];
   retval = ec_FPRD(0x1234, 0x1100, 256, data573_116, EC_TIMEOUTRXM);
   printf("232: FPRD 0x1100 TX: 0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 RX?: 0x00 \n");
   assert(retval == 1);
   ;
   osal_usleep(50000);
   printf("    Received: 0x");
   printHexCharArray(data573_116, 256);
   printf("\n");

   osal_usleep(50000);

   // ANDERS

   // scanf("%c",&ch);
   uint8_t data573_122[8] = {0x00, 0x12, 0x06, 0x00, 0x24, 0x00, 0x01, 0x00};
   retval = ec_FPWR(0x1234, 0x0810, 8, data573_122, EC_TIMEOUTRXM);
   printf("244: FPWR 0x0810 TX: 0x0001002400061200 \n");
   assert(retval == 1);

   // scanf("%c",&ch);
   uint8_t data573_123[8] = {0x00, 0x14, 0x0a, 0x00, 0x20, 0x00, 0x01, 0x00};
   retval = ec_FPWR(0x1234, 0x0818, 8, data573_123, EC_TIMEOUTRXM);
   printf("246: FPWR 0x0818 TX: 0x00010020000a1400 \n");
   assert(retval == 1);

   // scanf("%c",&ch);
   uint8_t data573_124[16] = {0x00, 0x00, 0x00, 0x01, 0x06, 0x00, 0x00, 0x07, 0x00, 0x12, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x0600, 16, data573_124, EC_TIMEOUTRXM);
   printf("248: FPWR 0x0600 TX: 0x00000001020012000700000601000000 \n");
   assert(retval == 1);

   // scanf("%c",&ch);
   uint8_t data573_126[16] = {0x00, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x00, 0x07, 0x00, 0x14, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00};
   retval = ec_FPWR(0x1234, 0x0610, 16, data573_126, EC_TIMEOUTRXM);
   printf("252: FPWR 0x0610 TX: 0x00000001010014000700000a01000000 \n");
   assert(retval == 1);

   // scanf("%c",&ch);
   uint8_t data573_127[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
   retval = ec_BWR(0x0000, 0x0300, 8, data573_127, EC_TIMEOUTRXM);
   printf("254: BWR 0x0300 TX: 0x0000000000000000 \n");
   assert(retval == 1);

   // scanf("%c",&ch);
   uint8_t data573_128[2] = {0x04, 0x00};
   retval = ec_FPWR(0x1234, 0x0120, 2, data573_128, EC_TIMEOUTRXM);
   printf("256: FPWR 0x0120 TX: 0x0004 \n");
   assert(retval == 1);
}

void transition_SO() {

   int retval;
   // scanf("%c",&ch);
   uint8_t data573_128[2] = {0x14, 0x00};
   retval = ec_FPWR(0x1234, 0x0120, 2, data573_128, EC_TIMEOUTRXM);
   printf("256: FPWR 0x0120 TX: 0x0008 \n");
   assert(retval == 1);

   uint8_t data[2];
   int success = 0;
   for (int i = 0; i < 1000; i++)
   {
      retval = ec_APRD(0x0, 0x130, 2, data, EC_TIMEOUTRXM);
      assert(retval == 1);

      if (memcmp(data, (uint8_t[]){0x04, 0x00}, 2) == 0)
      {
         success = 1;
         break;
      }

      osal_usleep(1000);
   }

   if (!success) {
      printf("Failed to transition to Safe-Operational\n");
      return;
   }

      uint8_t data573_198[2] = {0x08, 0x00};
   retval = ec_FPWR(0x1234, 0x0120, 2, data573_198, EC_TIMEOUTRXM);
   printf("256: FPWR 0x0120 TX: 0x0008 \n");
   assert(retval == 1);
   success = 0;
   for (int i = 0; i < 1000; i++)
   {
      retval = ec_APRD(0x0, 0x130, 2, data, EC_TIMEOUTRXM);
      assert(retval == 1);

      if (memcmp(data, (uint8_t[]){0x08, 0x00}, 2) == 0)
      {
         success = 1;
         break;
      }

      osal_usleep(1000);
   }

      if (!success) {
      printf("Failed to transition to Operational\n");
      return;
   }



   // Check status 
   uint8_t data573_129[4];
   retval = ec_FPRD(0x1234, 0x0130, 4, data573_129, EC_TIMEOUTRXM);
   printf("258: FPRD 0x0120 TX: 0x0000 RX?: 0x0008 \n");
   assert(retval == 1);

   printf("    Received: 0x");
   printHexCharArray(data573_129, 4);
   printf("\n");
   

}

int main(int argc, char *argv[])
{

   if (argc < 2)
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

      return -1;
   }

   char *ifname = argv[1];

   // Init socket
   if (!ec_init(ifname))
   {
      printf("No socket connection on %s\nExecute as root\n", ifname);
      return -1;
   }

   printf("Trans IP\n");

   transition_IP();

   // printf("Trans PS\n");

   transition_PS();

   transition_SO();

   ec_close();

   return 0;
}
