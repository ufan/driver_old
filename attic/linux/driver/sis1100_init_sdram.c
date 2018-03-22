/* $ZEL: sis1100_init_sdram.c,v 1.4 2003/01/09 12:13:16 wuestner Exp $ */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/pci.h>

#include <dev/pci/sis1100_sc.h>

#define SDRAM_EEPROM_CTRL_STAT  0x40000400

#define SDRAM_SCL    0x1
#define SDRAM_SDA    0x2
#define SDRAM_SDA_OE 0x4

static int
sis3100_spd_write(struct SIS1100_softc* sc, u_int32_t val)
{
    u_int32_t error;
    
    down(&sc->sem_hw);
    sis1100writereg(sc, t_hdr, 0x0f060402);
    wmb();
    sis1100writereg(sc, t_dal, val);
    wmb();
    sis1100writereg(sc, t_adl, SDRAM_EEPROM_CTRL_STAT);
    mb();
    do {
        error=sis1100readreg(sc, prot_error);
    } while (error==0x005);
    up(&sc->sem_hw);
    return error;
}

static int
sis3100_spd_read(struct SIS1100_softc* sc, u_int32_t* val)
{
    u_int32_t error;
    
    down(&sc->sem_hw);
    sis1100writereg(sc, t_hdr, 0x0f060002);
    wmb();
    sis1100writereg(sc, t_adl, SDRAM_EEPROM_CTRL_STAT);
    mb();
    do {
	error=sis1100readreg(sc, prot_error);
    } while (error==0x005);
    rmb();
    *val=sis1100readreg(sc, tc_dal);
    up(&sc->sem_hw);
    return error;
}

static int
sdram_eeprom_start(struct SIS1100_softc* sc)
{
    sis3100_spd_write(sc, 0);
    sis3100_spd_write(sc, SDRAM_SDA_OE|SDRAM_SDA);
    sis3100_spd_write(sc, SDRAM_SDA_OE|SDRAM_SDA|SDRAM_SCL);
    sis3100_spd_write(sc, SDRAM_SDA_OE|SDRAM_SCL);
    sis3100_spd_write(sc, SDRAM_SDA_OE);
    sis3100_spd_write(sc, 0) ;
    return 0;
}

static int
sdram_eeprom_stop(struct SIS1100_softc* sc)
{
  sis3100_spd_write(sc, 0);
  sis3100_spd_write(sc, SDRAM_SDA_OE);
  sis3100_spd_write(sc, SDRAM_SDA_OE|SDRAM_SCL);
  sis3100_spd_write(sc, SDRAM_SDA_OE|SDRAM_SDA|SDRAM_SCL);
  sis3100_spd_write(sc, SDRAM_SDA_OE|SDRAM_SDA);
  sis3100_spd_write(sc, 0);
  return 0;
}

static int
sdram_eeprom_read(struct SIS1100_softc* sc, int noack, u_int8_t* val)
{
    u_int32_t d;
    u_int8_t data;
    int i;

    data=0;
    for (i=0; i<8; i++) {
        sis3100_spd_write(sc, 0);
        sis3100_spd_write(sc, SDRAM_SCL);
        sis3100_spd_write(sc, SDRAM_SCL);
        sis3100_spd_read(sc, &d);

        data<<=1;
        data|=((d & 0x100)>>8);
    }

    *val=data;

    sis3100_spd_write(sc, noack?SDRAM_SDA_OE|SDRAM_SDA:SDRAM_SDA_OE);
    sis3100_spd_write(sc, noack?SDRAM_SDA_OE|SDRAM_SDA|SDRAM_SCL:SDRAM_SDA_OE|SDRAM_SCL);
    sis3100_spd_write(sc, noack?SDRAM_SDA_OE|SDRAM_SDA|SDRAM_SCL:SDRAM_SDA_OE|SDRAM_SCL);
    sis3100_spd_write(sc, noack?SDRAM_SDA_OE|SDRAM_SDA:SDRAM_SDA_OE);
    sis3100_spd_write(sc, 0);
    return 0 ;
}

static int
sdram_eeprom_write(struct SIS1100_softc* sc, u_int8_t val)
{
    u_int32_t data ;
    int i ;

    for (i=0; i<8; i++) {
        data=(val&0x80)?SDRAM_SDA_OE|SDRAM_SDA:SDRAM_SDA_OE;
        sis3100_spd_write(sc, data);
        sis3100_spd_write(sc, data);

        sis3100_spd_write(sc, data|SDRAM_SCL);

        sis3100_spd_write(sc, data);
        val<<=1;
    }

    sis3100_spd_write(sc, 0);
    sis3100_spd_write(sc, 0);
    sis3100_spd_write(sc, SDRAM_SCL);
    sis3100_spd_write(sc, SDRAM_SCL);
    sis3100_spd_write(sc, 0);
    return 0 ;
}

int
sis3100_init_sdram(struct SIS1100_softc* sc)
{
    u_int32_t eeprom_signature;
    u_int8_t eeprom_bytes[8];
    u_int8_t dummy;
    int i;

    sdram_eeprom_start(sc) ;
    sdram_eeprom_write(sc, 0xA0); /* device Write cmd  */
    sdram_eeprom_write(sc, 0x00); /* write address */

    sdram_eeprom_start(sc) ;

    sdram_eeprom_write(sc, 0xA1); /* device Read cmd  */

    for (i=0; i<8; i++) sdram_eeprom_read(sc, 0, eeprom_bytes+i);

    sdram_eeprom_read(sc, 1, &dummy);
    sdram_eeprom_stop(sc);
/*
    for (i=0; i<8; i++)
        printk(KERN_INFO "eeprom[%d]=0x%03x\n", i, eeprom_bytes[i]);
*/
    eeprom_signature=(eeprom_bytes[3]<<16)|(eeprom_bytes[4]<<8)|(eeprom_bytes[5]);
/*
    printk(KERN_INFO "eeprom_signature=0x%04x\n", eeprom_signature);
*/
    switch (eeprom_signature) {
    case 0x0c0901:
        sc->ram_size=64*1024*1024;
        break;
    case 0x0c0902:
        sc->ram_size=128*1024*1024;
        break;
    case 0x0d0a01:
        sc->ram_size=256*1024*1024;
        sis3100_spd_write(sc, 1<<16);
        break;
    case 0x0d0a02:
        sc->ram_size=512*1024*1024;
        sis3100_spd_write(sc, 1<<16);
        break;
    case 0xffffff:
        sc->ram_size=0;
        printk(KERN_INFO "SIS1100[%d]: no SDRAM installed\n", sc->unit);
        break;
    default:
        printk(KERN_ERR "SIS1100[%d]: SDRAM not supported: "
            "row=%d col=%d banks=%d\n",
                sc->unit, eeprom_bytes[3], eeprom_bytes[4], eeprom_bytes[5]);
        sc->ram_size=0;
    }
    return 0;
}
