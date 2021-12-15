#ifndef __TCB_H__
#define __TCB_H__

#include <stdint.h>

namespace aipudrv
{
enum
{
    TCB_INIT = 0,
    TCB_TASK,
    TCB_LOOP
};

enum
{
    TCB_NO_DEP = 0,
    TCB_IMMIDIATE_DEP,
    TCB_PRE_ALL_DEP
};

struct asx_conf_t
{
    uint32_t hi;
    uint32_t lo;
    uint32_t ctrl;
};

struct tcb_t
{
    uint32_t flag;
    uint32_t next;
    union
    {
        struct
        {
            uint32_t loop_count;
            uint32_t spc;
            uint32_t interrupt;
            uint16_t groupid;
            uint16_t gridid;
            uint16_t rsvd0;
            uint16_t taskid;
            uint16_t grid_dim_x;
            uint16_t grid_dim_y;
            uint16_t grid_dim_z;
            uint16_t group_dim_x;
            uint16_t group_dim_y;
            uint16_t group_dim_z;
            uint16_t group_id_x;
            uint16_t group_id_y;
            uint16_t group_id_z;
            uint16_t task_id_x;
            uint16_t task_id_y;
            uint16_t task_id_z;
            uint32_t sp;
            uint32_t pp;
            uint32_t dp;
            uint32_t cp;
            uint32_t pprint;
            uint32_t pprofiler;
            uint16_t coreid;
            uint16_t clusterid;
            uint16_t rsvd1;
            uint16_t tecid;
            uint32_t fmdp;
            uint32_t tap;
            uint32_t dap;
            uint32_t pap;
            uint32_t idp;
            uint32_t dsize;
            uint32_t rsvd2[5];
        } noninit;
        struct
        {
            uint16_t rsvd0[7];
            uint16_t gridid;
            uint32_t rsvd1;
            asx_conf_t asids[4];
            uint32_t gm_ctl;
            asx_conf_t gm[2];
            uint32_t rsvd2[6];
        } init;
    } __data;
};

#define loop_count __data.noninit.loop_count
#define spc __data.noninit.spc
#define interrupt __data.noninit.interrupt
#define groupid __data.noninit.groupid
#define gridid __data.noninit.gridid
#define taskid __data.noninit.taskid
#define grid_dim_x __data.noninit.grid_dim_x
#define grid_dim_y __data.noninit.grid_dim_y
#define grid_dim_z __data.noninit.grid_dim_z
#define group_dim_x __data.noninit.group_dim_x
#define group_dim_y __data.noninit.group_dim_y
#define group_dim_z __data.noninit.group_dim_z
#define group_id_x __data.noninit.group_id_x
#define group_id_y __data.noninit.group_id_y
#define group_id_z __data.noninit.group_id_z
#define task_id_x __data.noninit.task_id_x
#define task_id_y __data.noninit.task_id_y
#define task_id_z __data.noninit.task_id_z
#define sp __data.noninit.sp
#define pp __data.noninit.pp
#define dp __data.noninit.dp
#define cp __data.noninit.cp
#define pprint __data.noninit.pprint
#define pprofiler __data.noninit.pprofiler
#define coreid __data.noninit.coreid
#define clusterid __data.noninit.clusterid
#define tecid __data.noninit.tecid
#define fmdp __data.noninit.fmdp
#define tap __data.noninit.tap
#define dap __data.noninit.dap
#define pap __data.noninit.pap
#define idp __data.noninit.idp
#define dsize __data.noninit.dsize

#define igridid __data.init.gridid
#define asids __data.init.asids
#define gm_ctl __data.init.gm_ctl
#define gm __data.init.gm

#define TCB_FLAG_TASK_TYPE_INIT        0
#define TCB_FLAG_TASK_TYPE_TASK        1
#define TCB_FLAG_TASK_TYPE_LOOP_TASK   2

#define TCB_FLAG_DEP_TYPE_NONE         0
#define TCB_FLAG_DEP_TYPE_IMMEDIATE    (1 << 4)
#define TCB_FLAG_DEP_TYPE_PRE_ALL      (2 << 4)

#define TCB_FLAG_END_TYPE_NOT_END               0
#define TCB_FLAG_END_TYPE_END_WITHOUT_DESTROY   (1 << 6)
#define TCB_FLAG_END_TYPE_END_WITH_DESTROY      (2 << 6)
}

#endif //!__TCB_H__
