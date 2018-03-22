/* $ZEL: sis1100_mmap.c,v 1.4 2003/01/15 14:17:01 wuestner Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/select.h>
#include <sys/malloc.h>
#include <sys/callout.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <vm/vm.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/sis1100_var.h>
#include <dev/pci/sis1100_sc.h>

paddr_t
sis1100_mmap(dev_t dev, off_t off, int prot)
{
    struct SIS1100_softc *sc = SIS1100SC(dev);
    struct SIS1100_fdata* fd=SIS1100FD(dev);
    paddr_t addr=0;

    switch (fd->subdev) {
        case sis1100_subdev_remote:
            if (off>=sc->rem_size)
                return -1;
            addr=i386_btop(sc->rem_addr+off);
            break;
        case sis1100_subdev_ram:
            return -1;
        case sis1100_subdev_ctrl:
            if (off<sc->reg_size) {
                addr=i386_btop(sc->reg_addr+off);
            } else { /* mapped for DMA in userspace ? */
                if ((off>=fd->mmapdma.off) &&
                                (off-fd->mmapdma.off<fd->mmapdma.size))
                    addr=bus_dmamem_mmap(fd->mmapdma.dmat,
                            &fd->mmapdma.segs, 1,
                            off-fd->mmapdma.off,
                            VM_PROT_READ|VM_PROT_WRITE,
                            BUS_DMA_WAITOK|BUS_DMA_COHERENT);
                else
                    return -1;
            }
            break;
        case sis1100_subdev_dsp:
            return -1;
    }
    /*
    printf("mmap(dev=%d, %lld): 0x%08x\n", fd->subdev, off, (unsigned int)addr);
    */
    return addr;
}

int dma_alloc(struct SIS1100_softc *sc, struct SIS1100_fdata* fd,
    struct sis1100_dma_alloc* d)
{
    int rsegs, res;

    if (fd->mmapdma.valid) return EINVAL;

    fd->mmapdma.dmat=sc->sc_dmat;
    fd->mmapdma.size=d->size;
    fd->mmapdma.off=i386_round_page(sc->reg_size);

    res=bus_dmamem_alloc(fd->mmapdma.dmat, fd->mmapdma.size, 0, 0,
             &fd->mmapdma.segs, 1, &rsegs, BUS_DMA_WAITOK);
    if (res) {
        printf("%s: dmamem_alloc(%d) failed\n", sc->sc_dev.dv_xname, d->size);
        return res;
    }

    res=bus_dmamem_map(fd->mmapdma.dmat, &fd->mmapdma.segs, 1,
            fd->mmapdma.size, &fd->mmapdma.kva,
            BUS_DMA_WAITOK|BUS_DMA_COHERENT);
    if (res) {
        printf("%s: bus_dmamem_map(%ld) failed\n", sc->sc_dev.dv_xname,
                fd->mmapdma.size);
        return res;
    }

    res=bus_dmamap_create(fd->mmapdma.dmat, fd->mmapdma.size, 1,
             fd->mmapdma.size, 0, BUS_DMA_WAITOK,
             &fd->mmapdma.dm);
    if (res) {
        printf("%s: bus_dmamap_create(%ld) failed\n", sc->sc_dev.dv_xname,
                fd->mmapdma.size);
        return res;
    }

    res=bus_dmamap_load(fd->mmapdma.dmat, fd->mmapdma.dm, fd->mmapdma.kva,
             fd->mmapdma.size, NULL, BUS_DMA_WAITOK);
    if (res) {
        printf("%s: bus_dmamap_load(%ld) failed\n", sc->sc_dev.dv_xname,
                fd->mmapdma.size);
        return res;
    }
    d->size=fd->mmapdma.size;
    d->offset=fd->mmapdma.off;
    d->dma_addr=fd->mmapdma.segs.ds_addr;
    return 0;
}

int dma_free(struct SIS1100_softc *sc, struct SIS1100_fdata* fd,
    struct sis1100_dma_alloc* d)
{
    fd->mmapdma.valid=0;
    bus_dmamap_destroy(fd->mmapdma.dmat, fd->mmapdma.dm);
    bus_dmamem_unmap(fd->mmapdma.dmat, fd->mmapdma.kva, fd->mmapdma.size);
    bus_dmamem_free(fd->mmapdma.dmat, &fd->mmapdma.segs, 1);
    return 0;
}
