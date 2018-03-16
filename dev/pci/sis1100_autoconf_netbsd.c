/* $ZEL: sis1100_autoconf_netbsd.c,v 1.12 2008/01/17 15:55:06 wuestner Exp $ */

/*
 * Copyright (c) 2001-2006
 * 	Matthias Drochner, Peter Wuestner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <dev/pci/sis1100_sc.h>

struct sis1100_softc *sis1100_devdata[sis1100_MAXCARDS];

struct cdevsw sis1100cdevsw = {
	sis1100_open,
	sis1100_close,
	sis1100_read,
	sis1100_write,
	sis1100_ioctl,
	(dev_type_stop((*))) enodev,
	0,
	sis1100_poll,
	sis1100_mmap,
	0
};

#if __NetBSD_Version__ < 106000000
struct cfdriver sis1100cfdriver = {
	NULL,
	"sis1100_",
	DV_DULL,
	0
};
#else
struct cfdriver {
	LIST_ENTRY(cfdriver) cd_list;	/* link on allcfdrivers */
	struct cfattachlist cd_attach;	/* list of all attachments */
	void	**cd_devs;		/* devices found */
	const char *cd_name;		/* device name */
	enum	devclass cd_class;	/* device classification */
	int	cd_ndevs;		/* size of cd_devs array */
	const char * const *cd_attrs;	/* attributes for this device */
};
#endif

int sis1100match(struct device *, struct cfdata *, void *);
void sis1100attach(struct device *, struct device *, void *);
int sis1100detach(struct device *, int);

int sis1100intr(void *);

struct cfattach sis1100cfattach = {
	sizeof(struct sis1100_softc),
	sis1100match,
	sis1100attach,
	sis1100detach,
	0
};

int sis1100locs[] = {-1, -1};
extern const char *pcicf_locnames[];

struct cfdata sis1100cfdata = {
	&sis1100cfattach,
	&sis1100cfdriver,
	0,
	FSTATE_STAR,
	sis1100locs,
	0,
	NULL,
	pcicf_locnames
};

int
sis1100match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
#define MIN_FV 5
#define MAX_FV 7

	struct pci_attach_args *pa = aux;
        int revision;
	bus_space_tag_t reg_t;
	bus_space_handle_t reg_h;
	bus_size_t reg_size;
        u_int32_t ident;
        int hw_type, hw_ver, fw_code, fw_ver;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_FZJZEL ||
	    PCI_PRODUCT(pa->pa_id) != PCI_PRODUCT_FZJZEL_GIGALINK)
	        return (0);
        
        revision=PCI_REVISION(pci_conf_read(pa->pa_pc, pa->pa_tag,
                PCI_CLASS_REG));
        if (revision!=1) {
                printf("sis1100: pci_revision=%d\n", revision);
	        return (0);
        }

	if (pci_mapreg_map(pa, 0x18, PCI_MAPREG_TYPE_MEM, 0,
	    &reg_t, &reg_h, NULL, &reg_size)) {
		printf("sis1100: can't map register space\n");
		return 0;
	}
        ident=bus_space_read_4(reg_t, reg_h, ofs(struct sis1100_reg, ident));
        printf("sis1100: ident=0x%08x\n", ident);
        bus_space_unmap(reg_t, reg_h, reg_size);
        hw_type= ident     &0xff;
        hw_ver =(ident>> 8)&0xff;
        fw_code=(ident>>16)&0xff;
        fw_ver =(ident>>24)&0xff;
        if (hw_type!=1) {
                printf("sis1100: wrong hw_type %d\n", hw_type);
		return 0;
        }
        if (hw_ver!=1) {
                printf("sis1100: wrong hw_version %d\n", hw_ver);
		return 0;
        }
        if (fw_code!=1) {
                printf("sis1100: wrong fw_code %d\n", fw_code);
		return 0;
        }
        if (fw_ver<MIN_FV) {
                printf("sis1100: Firmware version (%d) too old;"
                        " at least version %d is required.\n", fw_ver, MIN_FV);
		return 0;
        }
        if (fw_ver>MAX_FV) {
                printf("sis1100: Firmware version (%d) too new;"
                    "Driver not tested with"
                    " versions greater than %d.\n", fw_ver, MAX_FV);
        }
	return (1);
#undef MIN_FV
#undef MAX_FV
}

void
sis1100attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sis1100_softc *sc = (void *)self;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	int i, flags;
	pci_intr_handle_t ih;
	const char *intrstr = NULL;

        printf("\n");
#if 0
        printf("MINORBITS    =%d\n", sis1100_MINORBITS);
        printf("MINORCARDBITS=%d\n", sis1100_MINORCARDBITS);
        printf("MAXCARDS     =%d\n", sis1100_MAXCARDS);
        printf("MINORUSERBITS=%d\n", sis1100_MINORUSERBITS);
        printf("MINORCARDMASK=0x%08x\n",sis1100_MINORCARDMASK );
        printf("MINORTYPEMASK=0x%08x\n", sis1100_MINORTYPEMASK);
        printf("MINORUSERMASK=0x%08x\n", sis1100_MINORUSERMASK);
        printf("MINORUTMASK  =0x%08x\n", sis1100_MINORUTMASK);
        printf("sc=%p\n", sc);
        printf("clearing fdatalist[0..%d]\n", sis1100_MINORUTMASK+1);
#endif

        for (i=0; i<=sis1100_MINORUTMASK; i++) sc->fdatalist[i]=0;

        if (pci_mapreg_info(pc, pa->pa_tag, 0x10, PCI_MAPREG_TYPE_MEM,
                &sc->plx_addr, &sc->plx_size, &flags)) {
            printf("%s: mapreg_info(plx) failed\n", sc->sc_dev.dv_xname);
        } else {
            printf("%s: mapreg_info(plx): size=%ld\n", sc->sc_dev.dv_xname,
                sc->plx_size);
        }
	if (pci_mapreg_map(pa, 0x10, PCI_MAPREG_TYPE_MEM, 0,
	    &sc->plx_t, &sc->plx_h, NULL, &sc->plx_size)) {
		printf("%s: can't map plx space\n", sc->sc_dev.dv_xname);
                sc->plx_size=0;
		return;
	}

        if (pci_mapreg_info(pc, pa->pa_tag, 0x18, PCI_MAPREG_TYPE_MEM,
                &sc->reg_addr, &sc->reg_size, &flags)) {
            printf("%s: mapreg_info failed\n", sc->sc_dev.dv_xname);
        } else {
            printf("%s: mapreg_info: size=%ld\n", sc->sc_dev.dv_xname,
                sc->reg_size);
        }
	if (pci_mapreg_map(pa, 0x18, PCI_MAPREG_TYPE_MEM, 0,
	    &sc->reg_t, &sc->reg_h, &sc->reg_addr, &sc->reg_size)) {
		printf("%s: can't map register space\n", sc->sc_dev.dv_xname);
                sc->reg_size=0;
		return;
	}

        if (pci_mapreg_info(pc, pa->pa_tag, 0x1c, PCI_MAPREG_TYPE_MEM,
                &sc->rem_addr, &sc->rem_size, &flags)) {
            printf("%s: mapreg_info failed\n", sc->sc_dev.dv_xname);
            sc->rem_size=0;
        } else {
            printf("%s: mapreg_info: size=%ld\n", sc->sc_dev.dv_xname,
                sc->rem_size);
        }

        if (sc->rem_size) {
	    if (pci_mapreg_map(pa, 0x1c, PCI_MAPREG_TYPE_MEM, 0,
	        &sc->rem_t, &sc->rem_h, &sc->rem_addr, &sc->rem_size)) {
		    printf("%s: can't map remote space\n", sc->sc_dev.dv_xname);
		    printf("%s: mmap not available\n", sc->sc_dev.dv_xname);
                    sc->rem_size=0;
                    sc->rem_addr=0;
	    }
        } else {
            sc->rem_addr=0;
        }

        pINFO(sc, "reg_addr=0x%08x, reg_size=%ld",
            (unsigned int)sc->reg_addr, sc->reg_size);
        pINFO(sc, "rem_addr=0x%08x, rem_size=%ld",
            (unsigned int)sc->rem_addr, sc->rem_size);

/*
        {
        int rev;
	rev = PCI_REVISION(pci_conf_read(pc, pa->pa_tag, PCI_CLASS_REG));
	printf("%s: PCI revision %d\n", sc->sc_dev.dv_xname, rev);
        }
*/
#ifdef NEVER
        {
        pcitag_t bridgetag;
        int i;

        bridgetag=pci_make_tag(pc, 0, 18, 0);
        for (i=0; i<0x40; i+=4) {
            pcireg_t x=pci_conf_read(pc, bridgetag, i);
            printf("bridge[%02x]: 0x%08x\n", i, x);
        }
        }
#endif
	sc->sc_pc = pc;
	sc->sc_pcitag = pa->pa_tag;
        sc->sc_dmat=pa->pa_dmat;

        simple_lock_init(&sc->lock_intcsr);
        simple_lock_init(&sc->lock_sc_inuse);
        simple_lock_init(&sc->handlercommand.lock);
        simple_lock_init(&sc->lock_doorbell);
        simple_lock_init(&sc->lock_lemo_status);
        simple_lock_init(&sc->remoteirq_wait);
        simple_lock_init(&sc->local_wait);
        simple_lock_init(&sc->demand_dma.spin);
        lockinit(&sc->sem_hw, 0, "sem_hw", 0, 0);
        lockinit(&sc->sem_fdata_list, 0, "sem_fdata_list", 0, 0);
        lockinit(&sc->sem_fdata_list, 0, "sem_irqinfo", 0, 0);
        lockinit(&sc->demand_dma.sem, 0, "demand_dma", 0, 0);

        sc->handlercommand.command=0;
        sc->demand_dma.status=dma_invalid;

        callout_init(&sc->link_up_timer);

        INIT_LIST_HEAD(&sc->fdata_list_head);
        kthread_create1(sis1100_irq_thread, sc, &sc->vmeirq_pp, "%s", "sis1100_irq");

        sc->sc_dma.devname=sc->sc_dev.dv_xname;
        sc->sc_dma.iot=sc->plx_t;
        sc->sc_dma.ioh=sc->plx_h;
        sc->sc_dma.dmat=pa->pa_dmat;

        if (plx9054_dmaalloc(&sc->sc_dma, MAX_DMA_LEN))
                return;

        sc->plxirq_dma0_hook=0;

	if (sis1100_init(sc)) {
                printf("%s: cannot initialize device\n", sc->sc_dev.dv_xname);
                return;
        }

	/* Map and establish the interrupt. */
	if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
			 pa->pa_intrline, &ih)) {
		printf("%s: couldn't map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);

	sc->sc_ih = pci_intr_establish(pc, ih, IPL_BIO, sis1100_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		       sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);
}

int
sis1100detach(self, flags)
	struct device *self;
	int flags;
{
	struct sis1100_softc *sc = (void *)self;
        int i;

	if (sc->sc_inuse)
		return (EBUSY);

	sis1100_done(sc);

	if (sc->sc_ih) {
		pci_intr_disestablish(sc->sc_pc, sc->sc_ih);
        }

        callout_stop(&sc->link_up_timer);
        simple_lock(&sc->handlercommand.lock);
        sc->handlercommand.command=handlercomm_die;
        wakeup(&sc->handler_wait);
        /* XXX PNORELOCK??? */
        ltsleep(sc, PCATCH|PNORELOCK, "terminate_sync", 0,
            &sc->handlercommand.lock);

        for (i=0; i<=sis1100_MINORUTMASK; i++) {
                if (sc->fdatalist[i]) {
                        free(sc->fdatalist[i], M_DEVBUF);
                }
        }

        plx9054_dmafree(&sc->sc_dma);

	if (sc->rem_size) {
                printf("unmap %ld bytes of rem\n", sc->rem_size);
		bus_space_unmap(sc->rem_t, sc->rem_h, sc->rem_size);
        }
	if (sc->reg_size) {
                printf("unmap %ld bytes of reg\n", sc->reg_size);
		bus_space_unmap(sc->reg_t, sc->reg_h, sc->reg_size);
        }
	if (sc->plx_size) {
                printf("unmap %ld bytes of plx\n", sc->plx_size);
		bus_space_unmap(sc->plx_t, sc->plx_h, sc->plx_size);
        }
	return (0);
}
