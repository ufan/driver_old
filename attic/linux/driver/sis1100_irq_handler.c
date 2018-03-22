/* $ZEL: sis1100_irq_handler.c,v 1.1 2003/01/09 12:16:04 wuestner Exp $ */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/wrapper.h>
#include <linux/pci.h>
#include <linux/list.h>
#include <asm/uaccess.h>

#include <dev/pci/sis1100_sc.h>

int
sis1100_irq_handler(void* data)
{
    struct SIS1100_softc* sc=(struct SIS1100_softc*)data;
    struct list_head* curr;
    enum handlercomm command;
    unsigned long flags;

    /*printk(KERN_INFO "sis1100_irq_handler started\n");*/

    daemonize();

    spin_lock_irq(&current->sig->siglock);
    sigemptyset(&current->blocked);
#if LINUX_VERSION_CODE < 0x20500
    recalc_sigpending(current);
#else
    recalc_sigpending();
#endif
    spin_unlock_irq(&current->sig->siglock);

    strncpy (current->comm, "SIS1100", sizeof(current->comm) - 1);
    current->comm[sizeof(current->comm) - 1] = '\0';

    while (1) {
        wait_event(
	    sc->handler_wait,
	    sc->handlercommand.command
	    );
        spin_lock_irqsave(&sc->handlercommand.lock, flags);
        command=sc->handlercommand.command;
        sc->handlercommand.command=0;
        spin_unlock_irqrestore(&sc->handlercommand.lock, flags);
/*
        printk(KERN_INFO "sis1100_irq_handler aufgewacht, command=0x%x j=%ld\n",
                command, jiffies);
*/
        sc->new_irqs=0;

        if (command&handlercomm_die) {
            /*printk(KERN_INFO "sis1100_irq_handler terminated\n");*/
            complete_and_exit(&sc->handler_completion, 0);
        }

        if (command&handlercomm_doorbell) {
            switch (sc->remote_hw) {
            case sis1100_hw_vme:
                sis3100rem_irq_handler(sc);
                break;
            case sis1100_hw_camac:
                sis5100rem_irq_handler(sc);
                break;
            case sis1100_hw_pci:     break; /* do nothing */
            case sis1100_hw_invalid: break; /* do nothing */
            }
        }

        if (command&handlercomm_lemo) {
            sis1100_lemo_handler(sc);
        }

        if (command&handlercomm_synch) {
            sis1100_synch_handler(sc);
        }

        list_for_each(curr, &sc->fdata_list_head) {
            struct SIS1100_fdata* fd;
            fd=list_entry(curr, struct SIS1100_fdata, list);
            if (fd->sig && (fd->sig!=-1) &&
                    ((sc->new_irqs & fd->owned_irqs)||
                            (sc->old_remote_hw!=sc->remote_hw))) {
                int res;
                res=kill_proc_info(fd->sig, (void*)0, fd->pid);
                if (res)
                    printk(KERN_WARNING "SIS1100[%d]: send sig %d to %d: res=%d\n",
                        sc->unit, fd->sig, fd->pid, res);
            }
        }

	if (signal_pending (current)) {
	    spin_lock_irq(&current->sig->siglock);
	    flush_signals(current);
	    spin_unlock_irq(&current->sig->siglock);
	}
    }
    return 0;
}

int
sis1100_irq_ctl(struct SIS1100_fdata* fd, struct sis1100_irq_ctl* data)
{
    struct SIS1100_softc* sc=fd->sc;
    int foreign_irqs;
    struct list_head* curr;

    if (data->signal) {
        foreign_irqs=0;
        down(&sc->sem_fdata_list);
        /* irq already in use? */
        list_for_each(curr, &sc->fdata_list_head) {
            struct SIS1100_fdata* fd;
            fd=list_entry(curr, struct SIS1100_fdata, list);
            foreign_irqs |= fd->owned_irqs;
        }
        up(&sc->sem_fdata_list);
        if (foreign_irqs & data->irq_mask) {
            printk(KERN_INFO "SIS1100[%d] irq_ctl: "
                    "IRQs owned by other programs: 0x%08x\n", 
                    sc->unit, foreign_irqs);
            return  -EBUSY;
        }

        fd->pid=current->pid;
        fd->sig=data->signal;
        fd->owned_irqs |= data->irq_mask;
        fd->old_remote_hw=sc->remote_hw;

        switch (sc->remote_hw) {
        case sis1100_hw_vme:
            sis3100rem_enable_irqs(fd, data->irq_mask);
            break;
        case sis1100_hw_camac:
            sis5100rem_enable_irqs(fd, data->irq_mask);
            break;
        case sis1100_hw_pci:     break; /* do nothing */
        case sis1100_hw_invalid: break; /* do nothing */
        }
        /* enable PCI-FRONT-IRQs */
        if (data->irq_mask & SIS1100_FRONT_IRQS) {
            u_int32_t mask;
            mask=(data->irq_mask & SIS1100_FRONT_IRQS)>>4;
            sis1100writereg(sc, cr, mask);
        }
    } else {
        int irqs;
        irqs=fd->owned_irqs & data->irq_mask;

        switch (sc->remote_hw) {
        case sis1100_hw_vme:
            sis3100rem_disable_irqs(fd, irqs);
            break;
        case sis1100_hw_camac:
            sis5100rem_disable_irqs(fd, irqs);
            break;
        case sis1100_hw_pci:     break; /* do nothing */
        case sis1100_hw_invalid: break; /* do nothing */
        }

        if (data->irq_mask & SIS1100_FRONT_IRQS) {
            u_int32_t mask;
            mask=(irqs & SIS1100_FRONT_IRQS)<<12;
            sis1100writereg(sc, cr, mask);
        }

        fd->owned_irqs &= ~data->irq_mask;
    }
    return 0;
}

int
sis1100_irq_ack(struct SIS1100_fdata* fd, struct sis1100_irq_ack* data)
{
    struct SIS1100_softc* sc=fd->sc;
    int irqs;

    irqs=fd->owned_irqs & data->irq_mask & sc->pending_irqs;
    sc->pending_irqs&=~irqs;

    switch (sc->remote_hw) {
    case sis1100_hw_vme:
        sis3100rem_irq_ack(sc, irqs);
        break;
    case sis1100_hw_camac:
        sis5100rem_irq_ack(sc, irqs);
        break;
    case sis1100_hw_pci:     break; /* do nothing */
    case sis1100_hw_invalid: break; /* do nothing */
    }

    return 0;
}

int
sis1100_irq_get(struct SIS1100_fdata* fd, struct sis1100_irq_get* data)
{
    struct SIS1100_softc* sc=fd->sc;

    data->irqs=sc->pending_irqs & fd->owned_irqs;
    if (fd->old_remote_hw!=sc->remote_hw) {
        if (sc->remote_hw!=sis1100_hw_invalid)
            data->remote_status=1;
        else
            data->remote_status=-1;
        fd->old_remote_hw=sc->remote_hw;
    } else
        data->remote_status=0;

    if (sc->remote_hw==sis1100_hw_vme)
        sis3100rem_get_vector(sc, data->irqs & data->irq_mask, data);
    else {
        data->level=0;
        data->vector=0;
    }
    return 0;
}

int
sis1100_irq_wait(struct SIS1100_fdata* fd, struct sis1100_irq_get* data)
{
        struct SIS1100_softc* sc=fd->sc;
        int irqs, res;

        irqs=fd->owned_irqs & data->irq_mask;

        res=wait_event_interruptible(sc->irq_wait,
                        ((sc->pending_irqs & irqs) ||
                        (fd->old_remote_hw!=sc->remote_hw)));
        if (res) return -EINTR;

        data->irqs=sc->pending_irqs & fd->owned_irqs;
        if (fd->old_remote_hw!=sc->remote_hw) {
                if (sc->remote_hw!=sis1100_hw_invalid)
                        data->remote_status=1;
                else
                        data->remote_status=-1;
                fd->old_remote_hw=sc->remote_hw;
        } else
                data->remote_status=0;

    if (sc->remote_hw==sis1100_hw_vme)
        sis3100rem_get_vector(sc, data->irqs & data->irq_mask, data);
    else {
        data->level=0;
        data->vector=0;
    }

        return 0;
}
