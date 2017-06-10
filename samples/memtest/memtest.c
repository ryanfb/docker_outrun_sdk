#include "memtest.h"
#include <stdlib.h> // NULL


/**********************************************************************
 *
 * Function:    memTestDataBus()
 *
 * Description: Test the data bus wiring in a memory region by
 *              performing a walking 1's test at a fixed address
 *              within that region.  The address (and hence the
 *              memory region) is selected by the caller.
 *
 * Notes:       
 *
 * Returns:     0 if the test succeeds.  
 *              A non-zero result is the first pattern that failed.
 *
 **********************************************************************/
unsigned char
memTestDataBus8(volatile unsigned char * address)
{
    unsigned char pattern;
    /*
     * Perform a walking 1's test at the given address.
     */
    for (pattern = 1; pattern != 0; pattern <<= 1)
    {
        /*
         * Write the test pattern.
         */
        *address = pattern;
        /*
         * Read it back (immediately is okay for this test).
         */
        if (*address != pattern)
        {
            return (pattern);
        }
    }
    return (0);
}   /* memTestDataBus8() */

unsigned short
memTestDataBus16(volatile unsigned short * address)
{
    unsigned short pattern;
    /*
     * Perform a walking 1's test at the given address.
     */
    for (pattern = 1; pattern != 0; pattern <<= 1)
    {
        /*
         * Write the test pattern.
         */
        *address = pattern;
        /*
         * Read it back (immediately is okay for this test).
         */
        if (*address != pattern)
        {
            return (pattern);
        }
    }
    return (0);
}   /* memTestDataBus8() */

/**********************************************************************
 *
 * Function:    memTestAddressBus()
 *
 * Description: Test the address bus wiring in a memory region by
 *              performing a walking 1's test on the relevant bits
 *              of the address and checking for aliasing. This test
 *              will find single-bit address failures such as stuck
 *              -high, stuck-low, and shorted pins.  The base address
 *              and size of the region are selected by the caller.
 *
 * Notes:       For best results, the selected base address should
 *              have enough LSB 0's to guarantee single address bit
 *              changes.  For example, to test a 64-Kbyte region,
 *              select a base address on a 64-Kbyte boundary.  Also,
 *              select the region size as a power-of-two--if at all
 *              possible.
 *
 * Returns:     NULL if the test succeeds.  
 *              A non-zero result is the first address at which an
 *              aliasing problem was uncovered.  By examining the
 *              contents of memory, it may be possible to gather
 *              additional information about the problem.
 *
 ********************************
**************************************/
// startOffset should be 2 if we're doing split 16 bit databus checks, and 1 for 8 bit checks.
datum *
memTestAddressBus(volatile datum * baseAddress, unsigned long nBytes, unsigned long startOffset)
{
    unsigned long addressMask = (nBytes/sizeof(datum) - 1);
    unsigned long offset;
    unsigned long testOffset;
    datum pattern     = (datum) 0xAAAAAAAA;
    datum antipattern = (datum) 0x55555555;
    /*
     * Write the default pattern at each of the power-of-two offsets.
     */
    for (offset = startOffset; (offset & addressMask) != 0; offset <<= 1)
    {
        baseAddress[offset] = pattern;
    }
    /*
     * Check for address bits stuck high.
     */
    testOffset = 0;
    baseAddress[testOffset] = antipattern;
    for (offset = startOffset; (offset & addressMask) != 0; offset <<= 1)
    {
        if (baseAddress[offset] != pattern)
        {
            return ((datum *) &baseAddress[offset]);
        }
    }
    baseAddress[testOffset] = pattern;
    /*
     * Check for address bits stuck low or shorted.
     */
    for (testOffset = startOffset; (testOffset & addressMask) != 0; testOffset <<= 1)
    {
        baseAddress[testOffset] = antipattern;
        if (baseAddress[0] != pattern)
        {
            return ((datum *) &baseAddress[testOffset]);
        }
        for (offset = startOffset; (offset & addressMask) != 0; offset <<= 1)
        {
            if ((baseAddress[offset] != pattern) && (offset != testOffset))
            {
                return ((datum *) &baseAddress[testOffset]);
            }
        }
        baseAddress[testOffset] = pattern;
    }
    return (NULL);
}   /* memTestAddressBus() */

/**********************************************************************
 *
 * Function:    memTestDevice()
 *
 * Description: Test the integrity of a physical memory device by
 *              performing an increment/decrement test over the
 *              entire region.  In the process every storage bit
 *              in the device is tested as a zero and a one.  The
 *              base address and the size of the region are
 *              selected by the caller.
 *
 * Notes:       
 *
 * Returns:     NULL if the test succeeds.  Also, in that case, the
 *              entire memory region will be filled with zeros.
 *
 *              A non-zero result is the first address at which an
 *              incorrect value was read back.  By examining the
 *              contents of memory, it may be possible to gather
 *              additional information about the problem.
 *
 **********************************************************************/
datum *
memTestDevice(volatile datum * baseAddress, unsigned long nBytes, unsigned long nSkip)
{
    unsigned long offset;
    unsigned long nWords = nBytes / sizeof(datum);
    datum pattern;
    datum antipattern;
    /*
     * Fill memory with a known pattern.
     */
    for (pattern = 1, offset = 0; offset < nWords; pattern++, offset += nSkip)
    {
        baseAddress[offset] = pattern;
    }
    /*
     * Check each location and invert it for the second pass.
     */
    for (pattern = 1, offset = 0; offset < nWords; pattern++, offset += nSkip)
    {
        if (baseAddress[offset] != pattern)
        {
            return ((datum *) &baseAddress[offset]);
        }
        antipattern = ~pattern;
        baseAddress[offset] = antipattern;
    }
    /*
     * Check each location for the inverted pattern and zero it.
     */
    for (pattern = 1, offset = 0; offset < nWords; pattern++, offset += nSkip)
    {
        antipattern = ~pattern;
        if (baseAddress[offset] != antipattern)
        {
            return ((datum *) &baseAddress[offset]);
        }
    }
    return (NULL);
}   /* memTestDevice() */

/**********************************************************************
 *
 * Function:    memTest()
 *
 * Description: Test a 64-k chunk of SRAM.
 *
 * Notes:       
 *
 * Returns:     0 on success.
 *              Otherwise -1 indicates failure.
 *
 **********************************************************************/

#if 0
int
memTest(void)
{
#define BASE_ADDRESS  (volatile datum *) 0x00000000
#define NUM_BYTES     64 * 1024
    if ((memTestDataBus(BASE_ADDRESS) != 0) ||
        (memTestAddressBus(BASE_ADDRESS, NUM_BYTES) != NULL) ||
        (memTestDevice(BASE_ADDRESS, NUM_BYTES) != NULL))
    {
        return (-1);
    }
    else
    {
        return (0);
    }
        
}   /* memTest() */
#endif