/* $ZEL: sis1100_autoconf.c,v 1.6 2003/01/15 14:16:59 wuestner Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/select.h>
#include <sys/malloc.h>
#include <sys/callout.h>
#include <sys/kthread.h>
#include <sys/signalvar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/sis1100_var.h>
#include <dev/pci/sis1100_sc.h>

#ifndef PCI_VENDOR_FZJZEL
#define PCI_VENDOR_FZJZEL 0x1796
#endif
#ifndef PCI_PRODUCT_FZJZEL_GIGALINK
#define PCI_PRODUCT_FZJZEL_GIGALINK 0x0001
#endif

struct cfdriver sis1100cfdriver = {
	NULL,
	"sis1100_",
	DV_DULL,
	0
};

int sis1100match(struct device *, struct cfdata *, void *);
void sis1100attach(struct device *, struct device *, void *);
int sis1100detach(struct device *, int);

int sis1100intr(void *);

struct cfattach sis1100cfattach = {
	sizeof(struct SIS1100_softc),
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
                printf("SIS1100: revision=%d\n", revision);
	        return (0);
        }

	if (pci_mapreg_map(pa, 0x18, PCI_MAPREG_TYPE_MEM, 0,
	    &reg_t, &reg_h, NULL, &reg_size)) {
		printf("SIS1100: can't map register space\n");
		return 0;
	}
        ident=bus_space_read_4(reg_t, reg_h, ofs(struct sis1100_reg, ident));
        printf("SIS1100: ident=0x%08x\n", ident);
        bus_space_unmap(reg_t, reg_h, reg_size);
        hw_type= ident     &0xff;
        hw_ver =(ident>> 8)&0xff;
        fw_code=(ident>>16)&0xff;
        fw_ver =(ident>>24)&0xff;
        if (hw_type!=1) {
                printf("SIS1100: wrong hw_type %d\n", hw_type);
		return 0;
        }
        if (hw_ver!=1) {
                printf("SIS1100: wrong hw_version %d\n", hw_ver);
		return 0;
        }
        if (fw_code!=1) {
                printf("SIS1100: wrong fw_code %d\n", fw_code);
		return 0;
        }
        if (fw_ver<MIN_FV) {
                printf("SIS1100: Firmware version (%d) too old;"
                        " at least version %d is required.\n", fw_ver, MIN_FV);
		return 0;
        }
        if (fw_ver>MAX_FV) {
                printf("SIS1100: Firmware version (%d) too new;"
                    "Driver not tested with"
                    " versions greater than %d.\n", fw_ver, MAX_FV);
		return 0;
        }
	return (1);
#undef MIN_FV
#undef MAX_FV
}

static int
plx9054_dmaalloc(sc, maxlen)
	struct plx9054dma *sc;
	int maxlen;
{
	bus_dma_tag_t dmat = sc->dmat;
	int res, rsegs;

	sc->nsegs = (maxlen + NBPG - 2) / NBPG + 1;
#ifdef PLXDEBUG
	printf("dma: allocating %d descriptors\n", sc->numdescs);
#endif

	res = bus_dmamem_alloc(dmat, sc->nsegs*16, 16, 0, &sc->descsegs, 1,
			       &rsegs, 0);
	if (res) {
		printf("%s: bus_dmamem_alloc failed\n", sc->devname);
		return (res);
	}
	res = bus_dmamem_map(dmat, &sc->descsegs, 1, sc->nsegs * 16,
			     (caddr_t *)&sc->descs, 0);
	if (res) {
		printf("%s: bus_dmamem_map failed\n", sc->devname);
		return (res);
	}

	res = bus_dmamap_create(dmat, sc->nsegs*16, 1, sc->nsegs*16, 0, 0,
				&sc->descdma);
	if (res) {
		printf("%s: bus_dmamap_create failed\n", sc->devname);
		return (res);
	}
	res = bus_dmamap_load(dmat, sc->descdma, sc->descs, sc->nsegs * 16,
			      NULL, 0);
	if (res) {
		printf("%s: bus_dmamap_load failed\n", sc->devname);
		return (res);
	}

	res = bus_dmamap_create(dmat, maxlen, sc->nsegs, maxlen, 0,
				BUS_DMA_WAITOK|BUS_DMA_ALLOCNOW, &sc->userdma);
	if (res) {
		printf("%s: bus_dmamap_create failed\n", sc->devname);
		return (res);
	}
        sc->userdma->dm_mapsize=0;
	return (0);
}

static void
plx9054_dmafree(sc)
	struct plx9054dma *sc;
{
	if (sc->descdma)
		bus_dmamap_destroy(sc->dmat, sc->descdma);
	if (sc->userdma)
		bus_dmamap_destroy(sc->dmat, sc->userdma);
	if (sc->descs)
		bus_dmamem_free(sc->dmat, &sc->descsegs, 1);
}

void
sis1100attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct SIS1100_softc *sc = (void *)self;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	int rev, i;
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

	if (pci_mapreg_map(pa, 0x10, PCI_MAPREG_TYPE_MEM, 0,
	    &sc->plx_t, &sc->plx_h, NULL, &sc->plx_size)) {
		printf("%s: can't map plx space\n", sc->sc_dev.dv_xname);
                sc->plx_size=0;
		return;
	}

	if (pci_mapreg_map(pa, 0x18, PCI_MAPREG_TYPE_MEM, 0,
	    &sc->reg_t, &sc->reg_h, &sc->reg_addr, &sc->reg_size)) {
		printf("%s: can't map register space\n", sc->sc_dev.dv_xname);
                sc->reg_size=0;
		return;
	}

	if (pci_mapreg_map(pa, 0x1c, PCI_MAPREG_TYPE_MEM, 0,
	    &sc->rem_t, &sc->rem_h, &sc->rem_addr, &sc->rem_size)) {
		printf("%s: can't map remote space\n", sc->sc_dev.dv_xname);
		printf("%s: mmap not available\n", sc->sc_dev.dv_xname);
                sc->rem_size=0;
	}
        printf("rem_addr=0x%08x, rem_size=%ld\n",
            (unsigned int)sc->rem_addr, sc->rem_size);

	rev = PCI_REVISION(pci_conf_read(pc, pa->pa_tag, PCI_CLASS_REG));
	printf("%s: PCI revision %d\n", sc->sc_dev.dv_xname, rev);

	sc->sc_pc = pc;
	sc->sc_pcitag = pa->pa_tag;
        sc->sc_dmat=pa->pa_dmat;

        simple_lock_init(&sc->lock_intcsr);
        simple_lock_init(&sc->lock_sc_inuse);
        lockinit(&sc->sem_hw, 0, "sem_hw", 0, 0);

        callout_init(&sc->link_up_timer);

        sc->sync_command.command=0;
        simple_lock_init(&sc->sync_command.lock);
        kthread_create1(sis1100thread_sync, sc, &sc->sync_pp, "%s", "sis1100_sync");
        sc->vmeirq_command.command=0;
        simple_lock_init(&sc->vmeirq_command.lock);
        kthread_create1(sis1100thread_vmeirq, sc, &sc->vmeirq_pp, "%s", "sis1100_vmeirq");
        simple_lock_init(&sc->vmeirq_wait);
        simple_lock_init(&sc->local_wait);

        sc->sc_dma.devname=sc->sc_dev.dv_xname;
        sc->sc_dma.iot=sc->plx_t;
        sc->sc_dma.ioh=sc->plx_h;
        sc->sc_dma.dmat=pa->pa_dmat;

        if (plx9054_dmaalloc(&sc->sc_dma, MAX_DMA_LEN))
                return;

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
	struct SIS1100_softc *sc = (void *)self;
        int i;

	if (sc->sc_inuse)
		return (EBUSY);

	sis1100_done(sc);
        callout_stop(&sc->link_up_timer);

        sc->sync_command.command=-1;
        wakeup(&sc->sync_pp);
        simple_lock(&sc->sync_command.lock);
        if (!sc->sync_command.command) {
            simple_unlock(&sc->sync_command.lock);
        } else {
            ltsleep(sc, PCATCH|PNORELOCK, "terminate_sync", 0,
                &sc->sync_command.lock);
        }

        simple_lock(&sc->vmeirq_command.lock);
        sc->vmeirq_command.command=-1;
        wakeup(&sc->vmeirq_pp);
        /* XXX PNORELOCK??? */
        ltsleep(sc, PCATCH|PNORELOCK, "terminate_vmeirq", 0,
                &sc->vmeirq_command.lock);

        for (i=0; i<=sis1100_MINORUTMASK; i++) {
                if (sc->fdatalist[i]) free(sc->fdatalist[i], M_DEVBUF);
        }

	if (sc->sc_ih)
		pci_intr_disestablish(sc->sc_pc, sc->sc_ih);

        plx9054_dmafree(&sc->sc_dma);

	if (sc->plx_size)
		bus_space_unmap(sc->plx_t, sc->plx_h, sc->plx_size);
	if (sc->reg_size)
		bus_space_unmap(sc->reg_t, sc->reg_h, sc->reg_size);
	if (sc->rem_size)
		bus_space_unmap(sc->rem_t, sc->rem_h, sc->rem_size);
	return (0);
}
/*
int
sis1100intr(vsc)
	void *vsc;
{
	return (sis1100_intr((struct SIS1100_softc *)vsc));
}
*/
