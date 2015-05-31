#define MSCF_GAIN_GROUP    0x00
#define MSCF_GAIN_COMMON   0x04
#define MSCF_THRESH_OFF    0x05
#define MSCF_THRESH_COMMON 0x15
#define MSCF_PZ_OFF        0x16
#define MSCF_PZ_COMMON     0x26
#define MSCF_SHAPE_GROUP   0x27
#define MSCF_SHAPE_COMMON  0x2b
#define MSCF_MULTI_HI      0x2c
#define MSCF_MULTI_LO      0x2d
#define MSCF_MONITOR       0x2e
#define MSCF_SINGLE_MODE   0x2f
#define MSCF_REMOTE        0x30 //Set Automatically by MADC_RC_OPCODE_ON or OFF

#define MSCF_GAIN_MASK     0x0f
#define MSCF_THRESH_MASK   0xff
#define MSCF_PZ_MASK       0xff
#define MSCF_SHAPE_MASK    0x03
#define MSCF_MULTI_MASK    0x08
#define MSCF_MONITOR_MASK  0x1f
#define MSCF_SINGLE_MASK   0x01
#define MSCF_REMOTE_MASK   0x01

