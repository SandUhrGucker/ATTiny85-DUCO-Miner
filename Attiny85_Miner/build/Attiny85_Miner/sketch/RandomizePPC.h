/*
  This code was taken from the stack-overflow answer from nergeia at:
  https://stackoverflow.com/questions/5355317/generate-a-random-number-using-the-c-preprocessor

  Only change I made to this code was the renaming of the KISS definition to SER_NO

  NOTE: This value will change at every re-compile assuming that at least one minute
  has elapsed since the previous compilation. I use this number as a pseudo-generic
  serial number for my AVR chips as a means of uniquely identifying themselves for
  serial communications with the ESP-Server.
*/
#ifndef _RANDOMIZE_PPC_H__
#define _RANDOMIZE_PPC_H__

#define UL      unsigned long
#define znew    ((z=36969*(z&65535)+(z>>16))<<16)
#define wnew    ((w=18000*(w&65535)+(w>>16))&65535)
#define MWC     (znew+wnew)
#define SHR3    (jsr=(jsr=(jsr=jsr^(jsr<<17))^(jsr>>13))^(jsr<<5))
#define CONG    (jcong=69069*jcong+1234567)
#define SER_NO  ((MWC^CONG)+SHR3)
/*  Global static variables: 
    (the seed changes on every minute) */
static UL z=362436069*(int)__TIMESTAMP__, w=521288629*(int)__TIMESTAMP__, \
  jsr=123456789*(int)__TIMESTAMP__, jcong=380116160*(int)__TIMESTAMP__;

#endif // _RANDOMIZE_PPC_H__
