/* Debug */
/* Provide printf subroutine and delay function */

#pragma once

#include <stdint.h>
#include "ch554.h"


void mDelayuS (uint16_t n); // Delay in units of uS
void mDelaymS (uint16_t n); // Delay in mS


/*******************************************************************************
* Function Name  : CfgFsys( )
* Description  : CH554 clock selection and configuration function, Fsys 6MHz is used by default, FREQ_SYS can be passed
                 CLOCK_CFG configuration, the formula is as follows:
                 Fsys = (Fosc * 4 / (CLOCK_CFG & MASK_SYS_CK_SEL); the specific clock needs to be configured by yourself
*******************************************************************************/ 
inline void CfgFsys( )  
{
	SAFE_MOD = 0x55;
	SAFE_MOD = 0xAA;
//     CLOCK_CFG |= bOSC_EN_XT;                          // Enable external crystal
//     CLOCK_CFG & = ~ bOSC_EN_INT;                      // Turn off the internal crystal

#if FREQ_SYS == 32000000
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x07;  // 32MHz
#elif FREQ_SYS == 24000000
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x06;  // 24MHz	
#elif FREQ_SYS == 16000000
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x05;  // 16MHz	
#elif FREQ_SYS == 12000000
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x04;  // 12MHz
#elif FREQ_SYS == 6000000
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x03;  // 6MHz	
#elif FREQ_SYS == 3000000
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x02;  // 3MHz	
#elif FREQ_SYS == 750000
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x01;  // 750KHz	
#elif FREQ_SYS == 187500
	CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x00;  // 187.5MHz		
#else
	#warning FREQ_SYS invalid or not set
#endif

	SAFE_MOD = 0x00;
}


/*******************************************************************************
* Function Name  : CH554WDTModeSelect(uint8_t mode)
* Description    : CH554 watchdog mode selection
* Input          : uint8_t mode
                   0  timer
                   1  watchDog
* Output         : None
* Return         : None
*******************************************************************************/
inline void CH554WDTModeSelect(uint8_t mode)
{
   SAFE_MOD = 0x55;
   SAFE_MOD = 0xaa;                                                             //Enter Safe Mode
   if(mode){
     GLOBAL_CFG |= bWDOG_EN;                                                    //Start watchdog reset
   }

   else GLOBAL_CFG &= ~bWDOG_EN;	                                            //Start watchdog only as a timer
   SAFE_MOD = 0x00;                                                             //exit safe Mode
   WDOG_COUNT = 0;                                                              //Watchdog assignment initial value

}

/*******************************************************************************
* Function Name  : CH554WDTFeed(uint8_t tim)
* Description    : CH554 watchdog timer time setting
* Input          : uint8_t tim watchdog reset time setting

                   00H(6MHz)=2.8s
                   80H(6MHz)=1.4s
* Output         : None
* Return         : None
*******************************************************************************/
inline void CH554WDTFeed(uint8_t tim)
{

   WDOG_COUNT = tim;                                                            // Watchdog counter assignment

}

// perform USB bus reset/disconnect
// set UDP to GPIO mode and hold low for device disconnect
inline void disconnectUSB()
{
    PIN_FUNC &= ~(bUSB_IO_EN);
    UDP = 0;
    mDelaymS(50);
    UDP = 1;
    PIN_FUNC |= bUSB_IO_EN;
}
