/*! \file bcmbd_schanfifo.c
 *
 * SCHAN FIFO driver APIs
 */
/*
 * Copyright: (c) 2018 Broadcom. All Rights Reserved. "Broadcom" refers to 
 * Broadcom Limited and/or its subsidiaries.
 * 
 * Broadcom Switch Software License
 * 
 * This license governs the use of the accompanying Broadcom software. Your 
 * use of the software indicates your acceptance of the terms and conditions 
 * of this license. If you do not agree to the terms and conditions of this 
 * license, do not use the software.
 * 1. Definitions
 *    "Licensor" means any person or entity that distributes its Work.
 *    "Software" means the original work of authorship made available under 
 *    this license.
 *    "Work" means the Software and any additions to or derivative works of 
 *    the Software that are made available under this license.
 *    The terms "reproduce," "reproduction," "derivative works," and 
 *    "distribution" have the meaning as provided under U.S. copyright law.
 *    Works, including the Software, are "made available" under this license 
 *    by including in or with the Work either (a) a copyright notice 
 *    referencing the applicability of this license to the Work, or (b) a copy 
 *    of this license.
 * 2. Grant of Copyright License
 *    Subject to the terms and conditions of this license, each Licensor 
 *    grants to you a perpetual, worldwide, non-exclusive, and royalty-free 
 *    copyright license to reproduce, prepare derivative works of, publicly 
 *    display, publicly perform, sublicense and distribute its Work and any 
 *    resulting derivative works in any form.
 * 3. Grant of Patent License
 *    Subject to the terms and conditions of this license, each Licensor 
 *    grants to you a perpetual, worldwide, non-exclusive, and royalty-free 
 *    patent license to make, have made, use, offer to sell, sell, import, and 
 *    otherwise transfer its Work, in whole or in part. This patent license 
 *    applies only to the patent claims licensable by Licensor that would be 
 *    infringed by Licensor's Work (or portion thereof) individually and 
 *    excluding any combinations with any other materials or technology.
 *    If you institute patent litigation against any Licensor (including a 
 *    cross-claim or counterclaim in a lawsuit) to enforce any patents that 
 *    you allege are infringed by any Work, then your patent license from such 
 *    Licensor to the Work shall terminate as of the date such litigation is 
 *    filed.
 * 4. Redistribution
 *    You may reproduce or distribute the Work only if (a) you do so under 
 *    this License, (b) you include a complete copy of this License with your 
 *    distribution, and (c) you retain without modification any copyright, 
 *    patent, trademark, or attribution notices that are present in the Work.
 * 5. Derivative Works
 *    You may specify that additional or different terms apply to the use, 
 *    reproduction, and distribution of your derivative works of the Work 
 *    ("Your Terms") only if (a) Your Terms provide that the limitations of 
 *    Section 7 apply to your derivative works, and (b) you identify the 
 *    specific derivative works that are subject to Your Terms. 
 *    Notwithstanding Your Terms, this license (including the redistribution 
 *    requirements in Section 4) will continue to apply to the Work itself.
 * 6. Trademarks
 *    This license does not grant any rights to use any Licensor's or its 
 *    affiliates' names, logos, or trademarks, except as necessary to 
 *    reproduce the notices described in this license.
 * 7. Limitations
 *    Platform. The Work and any derivative works thereof may only be used, or 
 *    intended for use, with a Broadcom switch integrated circuit.
 *    No Reverse Engineering. You will not use the Work to disassemble, 
 *    reverse engineer, decompile, or attempt to ascertain the underlying 
 *    technology of a Broadcom switch integrated circuit.
 * 8. Termination
 *    If you violate any term of this license, then your rights under this 
 *    license (including the license grants of Sections 2 and 3) will 
 *    terminate immediately.
 * 9. Disclaimer of Warranty
 *    THE WORK IS PROVIDED "AS IS" WITHOUT WARRANTIES OR CONDITIONS OF ANY 
 *    KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WARRANTIES OR CONDITIONS OF 
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE OR 
 *    NON-INFRINGEMENT. YOU BEAR THE RISK OF UNDERTAKING ANY ACTIVITIES UNDER 
 *    THIS LICENSE. SOME STATES' CONSUMER LAWS DO NOT ALLOW EXCLUSION OF AN 
 *    IMPLIED WARRANTY, SO THIS DISCLAIMER MAY NOT APPLY TO YOU.
 * 10. Limitation of Liability
 *    EXCEPT AS PROHIBITED BY APPLICABLE LAW, IN NO EVENT AND UNDER NO LEGAL 
 *    THEORY, WHETHER IN TORT (INCLUDING NEGLIGENCE), CONTRACT, OR OTHERWISE 
 *    SHALL ANY LICENSOR BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY DIRECT, 
 *    INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF 
 *    OR RELATED TO THIS LICENSE, THE USE OR INABILITY TO USE THE WORK 
 *    (INCLUDING BUT NOT LIMITED TO LOSS OF GOODWILL, BUSINESS INTERRUPTION, 
 *    LOST PROFITS OR DATA, COMPUTER FAILURE OR MALFUNCTION, OR ANY OTHER 
 *    COMMERCIAL DAMAGES OR LOSSES), EVEN IF THE LICENSOR HAS BEEN ADVISED OF 
 *    THE POSSIBILITY OF SUCH DAMAGES.
 */

#include <bsl/bsl.h>
#include <shr/shr_error.h>
#include <shr/shr_debug.h>
#include <sal_config.h>
#include <sal/sal_types.h>
#include <bcmdrd/bcmdrd_dev.h>
#include <bcmbd/bcmbd_schanfifo.h>
#include <bcmbd/bcmbd_internal.h>

#define BSL_LOG_MODULE  BSL_LS_BCMBD_SCHANFIFO

static schanfifo_ctrl_t *schanfifo_ctrl[SCHANFIFO_DEV_NUM_MAX];

int
bcmbd_schanfifo_attach(int unit)
{
    schanfifo_ctrl_t *ctrl = NULL;

    SHR_FUNC_ENTER(unit);

    ctrl = bcmbd_schanfifo_ctrl_get(unit);
    if (!ctrl) {
        SHR_IF_ERR_EXIT(SHR_E_UNAVAIL);
    }

    sal_memset(ctrl, 0, sizeof(*ctrl));
    ctrl->unit = unit;
    ctrl->active = 1;
    schanfifo_ctrl[unit] = ctrl;

exit:
    SHR_FUNC_EXIT();
}

int
bcmbd_schanfifo_detach(int unit)
{
    schanfifo_ctrl_t *ctrl = schanfifo_ctrl[unit];

    if (!ctrl) {
        return SHR_E_UNAVAIL;
    }

    if (!ctrl->active) {
        return SHR_E_NONE;
    }

    ctrl->active = 0;

    return SHR_E_NONE;
}

int
bcmbd_schanfifo_info_get(int unit, int *num_chans, size_t *cmd_mem_wsize)
{
    schanfifo_ctrl_t *ctrl = schanfifo_ctrl[unit];

    if (!ctrl || !ctrl->active || !ctrl->ops) {
        return SHR_E_UNAVAIL;
    }

    if (!num_chans || !cmd_mem_wsize) {
        return SHR_E_PARAM;
    }

    return ctrl->ops->info_get(ctrl, num_chans, cmd_mem_wsize);
}

int
bcmbd_schanfifo_init(int unit, uint32_t max_polls, uint32_t flags)
{
    schanfifo_ctrl_t *ctrl = schanfifo_ctrl[unit];

    if (!ctrl || !ctrl->active || !ctrl->ops) {
        return SHR_E_UNAVAIL;
    }

   return ctrl->ops->dev_init(ctrl, max_polls, flags);
}

int
bcmbd_schanfifo_ops_send(int unit, int chan, uint32_t num_ops,
                         uint32_t *req_buff, size_t req_wsize, uint32_t flags)
{
    schanfifo_ctrl_t *ctrl = schanfifo_ctrl[unit];

    if (!ctrl || !ctrl->active || !ctrl->ops) {
        return SHR_E_UNAVAIL;
    }

    if (chan < 0 || chan >= ctrl->channels || !num_ops ||
        !req_buff || req_wsize > ctrl->cmds_wsize) {
        return SHR_E_PARAM;
    }


   return ctrl->ops->ops_send(ctrl, chan, num_ops, req_buff, req_wsize, flags);
}

int
bcmbd_schanfifo_set_start(int unit, int chan, bool start)
{
    schanfifo_ctrl_t *ctrl = schanfifo_ctrl[unit];

    if (!ctrl || !ctrl->active || !ctrl->ops) {
        return SHR_E_UNAVAIL;
    }

    if (chan < 0 || chan >= ctrl->channels) {
        return SHR_E_PARAM;
    }

   return ctrl->ops->start_set(ctrl, chan, start);
}

int
bcmbd_schanfifo_status_get(int unit, int chan, uint32_t num_ops, uint32_t flags,
                           uint32_t *done_ops, uint32_t **resp_buff)
{
    schanfifo_ctrl_t *ctrl = schanfifo_ctrl[unit];

    if (!ctrl || !ctrl->active || !ctrl->ops) {
        return SHR_E_UNAVAIL;
    }

    if (chan < 0 || chan >= ctrl->channels || !num_ops || !done_ops) {
        return SHR_E_PARAM;
    }

   return ctrl->ops->status_get(ctrl, chan, num_ops, flags, done_ops, resp_buff);
}

