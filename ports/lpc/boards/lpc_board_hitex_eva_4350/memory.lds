/* Specify the memory areas */
MEMORY
{
    FLASH (rx)      : ORIGIN = 0x14000000, LENGTH = 1M
    RAM   (xrw)     : ORIGIN = 0x10000000, LENGTH = 128K
    RAMB  (rwx)     : ORIGIN = 0x10080000, LENGTH = 96K
}
