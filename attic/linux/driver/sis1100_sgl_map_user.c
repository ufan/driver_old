#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/pagemap.h>

/* The following functions are stolen from drivers/scsi/st.c. */

int
sgl_map_user_pages(struct scatterlist *sgl, const unsigned int max_pages, 
        const char* xuaddr, size_t count, int rw)
{
	int res, i, j;
	unsigned int nr_pages;
	struct page **pages;
        unsigned long uaddr=(unsigned long)xuaddr;

	nr_pages = ((uaddr & ~PAGE_MASK) + count + ~PAGE_MASK) >> PAGE_SHIFT;

	/* User attempted Overflow! */
	if ((uaddr + count) < uaddr)
		return -EINVAL;

	/* Too big */
        if (nr_pages > max_pages) {
                printk(KERN_ERR
                    "sgl_map_user_pages: nr_pages=%d but max_pages=%d\n",
                        nr_pages, max_pages);
		return -ENOMEM;
        }
	/* Hmm? */
	if (count == 0)
		return 0;

	if ((pages = kmalloc(nr_pages * sizeof(*pages), GFP_KERNEL)) == NULL) {
                printk(KERN_ERR
                    "sgl_map_user_pages: kmalloc(%d*sizeof(struct page*)) failed\n",
                        nr_pages);
		return -ENOMEM;
        }

        /* Try to fault in all of the necessary pages */
	down_read(&current->mm->mmap_sem);
        /* rw==READ means read from drive, write into memory area */
	res = get_user_pages(
		current,
		current->mm,
		uaddr,
		nr_pages,
		rw == READ,
		0, /* don't force */
		pages,
		NULL);
	up_read(&current->mm->mmap_sem);

	/* Errors and no page mapped should return here */
	if (res < nr_pages) {
                printk(KERN_ERR "sgl_map_user_pages: nr_pages=%d; res=%d\n",
                    nr_pages, res);
		goto out_unmap;
        }

        for (i=0; i < nr_pages; i++) {
                /* FIXME: flush superflous for rw==READ,
                 * probably wrong function for rw==WRITE
                 */
		flush_dcache_page(pages[i]);
        }

	/* Populate the scatter/gather list */
#if LINUX_VERSION_CODE < 0x20500
        sgl[0].address=0;
#endif
	sgl[0].page = pages[0]; 
	sgl[0].offset = uaddr & ~PAGE_MASK;
	if (nr_pages > 1) {
		sgl[0].length = PAGE_SIZE - sgl[0].offset;
#ifdef CONFIG_SPARC64
                sgl[0].dma_length=0;
#endif
		count -= sgl[0].length;
		for (i=1; i < nr_pages ; i++) {
#if LINUX_VERSION_CODE < 0x20500
                        sgl[i].address=0;
#endif
			sgl[i].offset = 0;
			sgl[i].page = pages[i]; 
			sgl[i].length = count < PAGE_SIZE ? count : PAGE_SIZE;
#ifdef CONFIG_SPARC64
                        sgl[i].dma_length=0;
#endif
			count -= PAGE_SIZE;
		}
	}
	else {
		sgl[0].length = count;
#ifdef CONFIG_SPARC64
                sgl[0].dma_length=0;
#endif
	}
for (i=0; i<nr_pages; i++) {
    if (sgl[i].length<0) {
        printk(KERN_ERR "sgl[%d].length=%d\n", i, sgl[i].length);
        res=-EIO;
        goto out_unmap;
    }
}
	kfree(pages);
	return nr_pages;

 out_unmap:
	if (res > 0) {
		for (j=0; j < res; j++)
			page_cache_release(pages[j]);
                res=-EIO;
	}
	kfree(pages);
	return res;
}


/* And unmap them... */
int
sgl_unmap_user_pages(struct scatterlist *sgl, const unsigned int nr_pages,
        int dirtied)
{
	int i;

	for (i=0; i < nr_pages; i++) {
		if (dirtied && !PageReserved(sgl[i].page))
			SetPageDirty(sgl[i].page);
		/* FIXME: cache flush missing for rw==READ
		 * FIXME: call the correct reference counting function
		 */
		page_cache_release(sgl[i].page);
	}

	return 0;
}

#ifdef CONFIG_SPARC64
void
dump_sgl(struct scatterlist *sgl, int nr_pages)
{
	int i;

	for (i=0; i<nr_pages; i++) {
            printk(KERN_ERR "sgl[%d]: page=%p offs=%d len=%d dma_len=%d\n",
                i, sgl[i].page, sgl[i].offset, sgl[i].length, sgl[i].dma_length);
        }
}
#endif
