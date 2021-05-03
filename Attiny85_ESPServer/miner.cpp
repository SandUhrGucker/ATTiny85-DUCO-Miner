#include "miner.h"

unsigned int DUCO_Miner::BASE_MINER_ID = 0;

String DUCO_Miner::jobs[] = {
  "0000000000000000000000000000000000000000,13f23aa0bff731803eb9d4a000ce37d067301dd4,6",
  "13f23aa0bff731803eb9d4a000ce37d067301dd4,45db4ef2a7a5ce029e4d12065ab80d10e44ce07a,6",
  "45db4ef2a7a5ce029e4d12065ab80d10e44ce07a,9f0c10a7a16b84f54e148e7c7c4e3a9b591f624f,6",
  "9f0c10a7a16b84f54e148e7c7c4e3a9b591f624f,9001c6ac34eb185cdb1cabde89acf4275f39b593,6"
};

unsigned int DUCO_Miner::answers[] = { 1,8, 600, 480 };
