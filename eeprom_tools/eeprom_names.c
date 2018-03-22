/*
 * $ZEL: eeprom_names.c,v 1.2 2008/11/25 15:28:41 wuestner Exp $
 */

#define _GNU_SOURCE

#include <sys/types.h>
#include "plx_eeprom.h"

char* eeprom_names[EEPROM_LEN_8311]= {
    "Device ID",
    "Vendor ID",
    "Class Code",
    "Class Code/Revision",
    "Maximum Latency/Minimum Grant",
    "Interrupt Pin/Interrupt Line Routing",
    "MSW of Mailbox 0",
    "LSW of Mailbox 0",
    "MSW of Mailbox 1",
    "LSW of Mailbox 1",
    "MSW of Range for PCI-to-Local Address Space 0",
    "LSW of Range for PCI-to-Local Address Space 0",
    "MSW of Local Base Address for PCI-to-Local Address Space 0",
    "LSW of Local Base Address for PCI-to-Local Address Space 0",
    "MSW of Mode/DMA Arbitration Register",
    "LSW of Mode/DMA Arbitration Register",
    "MSW of Serial EEPROM Write-Protected Address",
    "LSW of Local Misc. Control Register/Local Big/Little Endian",
    "MSW of Range for PCI-to-Local Expansion ROM",
    "LSW of Range for PCI-to-Local Expansion ROM",
    "MSW of Local Base Address for PCI-to-Local Expansion ROM",
    "LSW of Local Base Address for PCI-to-Local Expansion ROM",
    "MSW of Bus Region Desciptors for PCI-to-Local Accesses",
    "LSW of Bus Region Desciptors for PCI-to-Local Accesses",
    "MSW of Range for PCI Initiator-to-PCI",
    "LSW of Range for PCI Initiator-to-PCI",
    "MSW of Local Base Address for PCI Initiator-to-PCI Memory",
    "LSW of Local Base Address for PCI Initiator-to-PCI Memory",
    "MSW of Local Bus Address for PCI Initiator-to-PCI I/O Configuation",
    "LSW of Local Bus Address for PCI Initiator-to-PCI I/O Configuation",
    "MSW of PCI Base Address for PCI Initiator-to-PCI",
    "LSW of PCI Base Address for PCI Initiator-to-PCI",
    "MSW of PCI Config. Addr. Reg. for PCI Initiator-to-PCI I/O Config.",
    "LSW of PCI Config. Addr. Reg. for PCI Initiator-to-PCI I/O Config.",
    "Subsystem ID",
    "Subsystem Vendor ID",
    "MSW of Range for PCI-to-Local Address Space 1",
    "LSW of Range for PCI-to-Local Address Space 1",
    "MSW of Local Base Address for PCI-to-Local Address Space 1",
    "LSW of Local Base Address for PCI-to-Local Address Space 1",
    "MSW of Bus Region Desciptors (Space 1) for PCI-to-Local Accesses",
    "LSW of Bus Region Desciptors (Space 1) for PCI-to-Local Accesses",
    "MSW of Hot Swap Control/Status",
    "LSW of Hot Swap Control / Hot Swap Next Capability Pointer",
    "reserved",
    "Internal Arbiter Control",
    "Power Management Capabilities",
    "Power Management Next Capability Pointer",
    "Power Management Data",
    "Power Management Control/Status",
};

#ifdef USE_SERIAL
char* eeprom_names_ser[2]= {
    "CMC_Type 0: 5V 1: 3.3V 2: both 3: PCIe+OPT 4: PCIe single link 5: PCIe quad link",
    "Serial No.",
};
#endif
