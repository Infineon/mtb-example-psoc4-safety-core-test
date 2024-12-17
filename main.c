/******************************************************************************
* File Name: main.c
*
* Description:This file provides the following core peripheral self tests
*             for PSoC 4 :
*             - CPU registers test
*             - Program Counter test
*             - WDT test
*             - Clock test
*             - Interrupt test
*             - IO test
*             - Flash test (fletcher's test + CRC test)
*             - Config Registers test
*             - SRAM/Stack test (March test)
*             - Stack Overflow test
*
* Related Document: See README.md
*
********************************************************************************
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
********************************************************************************/

/*******************************************************************************
* Includes
********************************************************************************/

#include "cy_pdl.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

#include "SelfTest_Clock.h"
#include "SelfTest_WDT.h" 
#include "SelfTest_CPU.h"
#include "SelfTest_Interrupt.h"
#include "SelfTest_Flash.h"
#include "SelfTest_RAM.h"
#include "SelfTest_Stack.h"
#include "SelfTest_IO.h"
#include "SelfTest_ConfigRegisters.h"


/*******************************************************************************
* Global Variables
*******************************************************************************/
/*Index for IPs*/
uint8_t ip_index = 1u;
/* Array to set shifts for March RAM test. */
uint8_t stack_restore_buff[4096] = {0u};
#if defined (__ICCARM__)
#if (FLASH_TEST_MODE == FLASH_TEST_FLETCHER64)
CY_SECTION(".flash_checksum") const uint64_t flash_StoredCheckSum = 0x3AA72D8E46BE6A0D;
#endif
#if (FLASH_TEST_MODE == FLASH_TEST_CRC32)
CY_SECTION(".flash_checksum") const uint64_t flash_StoredCheckSum = 0x2bb718d655ca26d;
#endif
#else
#if (FLASH_TEST_MODE == FLASH_TEST_FLETCHER64)
CY_SECTION(".flash_checksum")  uint64_t flash_StoredCheckSum = 0x19AF3D7B846D3685;
#endif
#if (FLASH_TEST_MODE == FLASH_TEST_CRC32)
CY_SECTION(".flash_checksum") uint64_t flash_StoredCheckSum = 0x2bb718d655ca26d;
#endif
#endif

#if defined (__ICCARM__)
#pragma optimize=none
void test()
{
    SelfTest_Flash_init(CY_FLASH_BASE,FLASH_END_ADDR,flash_StoredCheckSum);
}
#endif

/*****************************************************************************
* Function Name: IAR_Flash_Init
******************************************************************************
* Summary:
* The function ensures that compiler optimizations are not done for IAR
* compiler in the release mode.
*
* Parameters:
*  void
*
* Return:
*  void
*****************************************************************************/
#if defined (__ICCARM__)
#pragma optimize=none
void IAR_Flash_Init()
{
    SelfTest_Flash_init(CY_FLASH_BASE,FLASH_END_ADDR,flash_StoredCheckSum);
}
#endif

/*******************************************************************************
* Macros
********************************************************************************/
/*The size of RAM/ STACK block to be tested. */
#define BLOCK_SIZE                1024
/*The size of buffer which is used to store/restore. */
#define BUFFER_SIZE               1024
#define MAX_INDEX_VAL (0xFFF0u)
#define PATTERN_BLOCK_SIZE (8u)
#define DEVICE_SRAM_BASE     (0x20000000)
#define DEVICE_STACK_SIZE    (0x00000400)
#define DEVICE_SRAM_SIZE     (0x00008000)
#define DEVICE_STACK_BASE    (DEVICE_SRAM_BASE + DEVICE_SRAM_SIZE)


/* Print Test Result*/
#define PRINT_TEST_RESULT(test_name, status) \
    do { \
        if (OK_STATUS == ret) { \
            /* Process success */ \
            printf("\r\n%s test: success\r\n\n", (test_name)); \
        } \
        else if (PASS_COMPLETE_STATUS == ret) { \
            /* Process status */ \
            printf("\r\n%s test: success\r\n\n", (test_name)); \
            break; \
        } \
        else if (PASS_STILL_TESTING_STATUS == ret) { \
            /* Print test counter */ \
            printf("\rTesting %s... count=%d", (test_name), count); \
        } \
        else { \
            /* Process error */ \
            printf("\r\n%s test: error", (test_name)); \
            if ((status)) \
                printf(": %d", (status)); \
            printf("\r\n"); \
        } \
    } while (0)

/*******************************************************************************
* Global Variables
********************************************************************************/
/* SelfTest API return status */
static uint8_t ret = 0u;;

static uint16_t count = 0u;

/*******************************************************************************
* Function Prototypes
********************************************************************************/
static void Clock_Test(void);
static void Flash_Test(void);
static void Interrupt_Test(void);
static void IO_Test(void);
static void SRAM_March_Test(void);
static void Stack_March_Test(void);
static void Stack_Memory_Test(void);
static void Interrupt_test_Init(void);
static void clock_test_init(void);
static void Start_Up_Test(void);

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function. It does...
*    1. Initialize the device and board peripherals and retarget-io for prints
*    2. Calls the test APIs for testing the followings:
*        - Program Counter
*        - CPU registers
*        - WDT
*        - Clock
*        - Interrupt
*        - Flash
*        - IO
*        - Config Registers test
*        - SRAM/Stack test (March test)
*        - Stack Overflow test
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/


int main(void)
{
    cy_rslt_t result;


    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");
    printf("\r\nClass-B Safety Test: Core Peripheral Resources\r\n");

    /* Start Up Test */
    Start_Up_Test();

    /* Program counter Test */
    ret = SelfTest_PC();
    
    PRINT_TEST_RESULT("Program counter", ret);

    /* CPU Registers Test*/
    ret = SelfTest_CPU_Registers();
 
    PRINT_TEST_RESULT("CPU registers", ret);

    /* Watch Dog Timer Test */
    ret = SelfTest_WDT();
    
    PRINT_TEST_RESULT("WDT", ret);

    Clock_Test();

    Interrupt_Test();

    Flash_Test();

    IO_Test();

#if (STARTUP_CFG_REGS_MODE == CFG_REGS_TO_FLASH_MODE)
    //Save_Startup_Registers();
#endif /* End (STARTUP_CFG_REGS_MODE == CFG_REGS_TO_FLASH_MODE) */

    SRAM_March_Test();

    Stack_March_Test();

    /* Stack Overflow and Underflow Test */
    Stack_Memory_Test();


    /***************************************************************************
     * Run-time test to verify whether the stack overflowed or configuration
     * registers are not corrupted.
     **************************************************************************/

    for (;;)
    {
        /* Print test counter */
        printf("\rPerforming run-time tests.. count: %d", count);

        count++;

        if (count > MAX_INDEX_VAL)
        {
            count = 0u;
        }
    }
}

/*****************************************************************************
* Function Name: Clock_Test
******************************************************************************
* Summary:
* Clock Test : Testing clock frequency using Independent Time slot
* monitoring technique
*
* Parameters:
*  void
*
* Return:
*  void
*****************************************************************************/
static void Clock_Test(void)
{
    clock_test_init();

    for (;;)
    {
        ret = SelfTest_Clock(CYBSP_CLOCK_TEST_TIMER_HW, CYBSP_CLOCK_TEST_TIMER_NUM);

        PRINT_TEST_RESULT("Clock", ret);

        if (PASS_STILL_TESTING_STATUS != ret) {
            break;
        }

        count++;
        if (count > MAX_INDEX_VAL){
            count = 0u;
        }
    }
    /* Either you need to clear WDT interrupt periodically or
     * disable it to ensure no WDT reset */
    Cy_WDT_ClearInterrupt();
    Cy_WDT_Disable();
}

/*****************************************************************************
* Function Name: Memory_Test
******************************************************************************
* Summary:
* Memory Test: Testing memory using Self Tests...
*
* Parameters:
*  void
*
* Return:
*  void
*****************************************************************************/
static void Stack_Memory_Test(void)
{
    /* Init Stack SelfTest */
    SelfTests_Init_Stack_Range((uint16_t*)DEVICE_STACK_BASE, DEVICE_STACK_SIZE, PATTERN_BLOCK_SIZE);
    if(ERROR_STATUS == ret)
    {
        printf("\r\n");
    }
    /*******************************/
    /* Run Stack Self Test...      */
    /*******************************/
    uint8_t ret = SelfTests_Stack_Check_Range((uint16_t*)DEVICE_STACK_BASE, DEVICE_STACK_SIZE);
    if ((ERROR_STACK_OVERFLOW & ret))
    {
         /* Process error */
        PRINT_TEST_RESULT("Stack Overflow", ret);
    }
    else if ((ERROR_STACK_UNDERFLOW & ret))
    {
        ret = ERROR_STATUS;
         /* Process error */
        PRINT_TEST_RESULT("Stack Underflow", ret);
    }

    else
    {
        PRINT_TEST_RESULT("Stack Memory", ret);
    }
    ip_index++;
}

/*****************************************************************************
* Function Name: Flash_Test
******************************************************************************
* Summary:
* Flash Test : Testing the flash by comparing the stored checksum in
* flash with calculated checksum of the data stored in flash.
*
* Parameters:
*  void
*
* Return:
*  void
*****************************************************************************/
static void Flash_Test(void)
{
    /* Variable for output calculated Flash Checksum */
    uint8_t flash_CheckSum_temp;
#if defined (__ICCARM__)
    IAR_Flash_Init();
#else
    SelfTest_Flash_init(CY_FLASH_BASE,FLASH_END_ADDR,flash_StoredCheckSum);
#endif

    for(;;)
    {
        ret = SelfTest_FlashCheckSum(FLASH_DOUBLE_WORDS_TO_TEST);

        PRINT_TEST_RESULT("Flash", ret);

        if (ERROR_STATUS == ret)
        {
#if(FLASH_TEST_MODE == FLASH_TEST_CRC32)
            printf("\r\nFLASH CRC: 0x");
#elif (FLASH_TEST_MODE == FLASH_TEST_FLETCHER64)
            printf("\r\nFLASH CHECKSUM: 0x");
#endif

            /* Output calculated Flash Checksum */
            for(int16_t i = sizeof(flash_CheckSum) - 1; i >= 0; i--)
            {
                flash_CheckSum_temp = (uint8_t) (flash_CheckSum >> (i*8u));
                printf("%02X", flash_CheckSum_temp);
            }
            printf("\r\n");
            break;
        }
        else if (PASS_COMPLETE_STATUS == ret) {
            break;
        }
        else
        {
            /* Do Nothing */
        }
        count++;
        if (count > MAX_INDEX_VAL) {
            count = 0u;
        }
    }
}



/*****************************************************************************
* Function Name: Interrupt_Test
******************************************************************************
* Summary:
* Interrupt Test : Testing Interrupt controller using independent time
* slot monitoring technique
*
* Parameters:
*  void
*
* Return:
*  void
*****************************************************************************/
static void Interrupt_Test(void)
{
    Interrupt_test_Init();

    ret = SelfTest_Interrupt(CYBSP_TIMER_HW, CYBSP_TIMER_NUM);

    PRINT_TEST_RESULT("Interrupt", ret);
}


/*****************************************************************************
* Function Name: IO_Test
******************************************************************************
* Summary:
* IO Test : Testing IO functionality by writing 1/0 to each pin and
* then reading it back.
*
* Parameters:
*  void
*
* Return:
*  void
*****************************************************************************/
static void IO_Test(void)
{
    char uart_debug_string[16];
    ret = SelfTest_IO();
    if (OK_STATUS != ret)
    {
        printf("\r\nIO test: error | retruns %d\r\n\n", ret);
        sprintf(uart_debug_string,"PORT %d[%d]",SelfTest_IO_GetPortError(),SelfTest_IO_GetPinError());
        printf(uart_debug_string);
    }
    else
    {
        printf("\r\nIO test: success\r\n\n");

    }

}


/*****************************************************************************
* Function Name: SRAM_March_Test
******************************************************************************
* Summary:
* SRAM March Test: Testing SRAM using March Self Tests...
*
* Parameters:
*  void
*
* Return:
*  void
*****************************************************************************/
static void SRAM_March_Test(void)
{
    uint8_t restore_buff[BUFFER_SIZE] = {0u};
    if(ERROR_STATUS == ret)
    {
        printf("\r\n");
    }
    __disable_irq();

    ret = SelfTest_SRAM(SRAM_MARCH_TEST_MODE,(uint8_t *)DEVICE_SRAM_BASE,BLOCK_SIZE,(uint8_t *)restore_buff,BUFFER_SIZE);

    __enable_irq();

    PRINT_TEST_RESULT("SRAM March", ret);

}

/*****************************************************************************
* Function Name: Stack_March_Test
******************************************************************************
* Summary:
* Stack March Test: Testing Stack using March Self Tests.
*
* Parameters:
*  void
*
* Return:
*  void
*****************************************************************************/
static void Stack_March_Test(void)
{
    __disable_irq();

    ret = SelfTest_SRAM_Stack((uint8_t *)DEVICE_STACK_BASE,(uint32_t)DEVICE_STACK_SIZE,stack_restore_buff);

    __enable_irq();

     /*Process error*/
    PRINT_TEST_RESULT("Stack March", ret);
}


/******************************************************************************
* Function Name: Interrupt_test_Init
*******************************************************************************
*
* Summary: Initialize the Timer interrupt for Interrupt Self test.
*
* Parameters:
*  void
*
* Return:
*  void
*
******************************************************************************/
static void Interrupt_test_Init(void)
{
    cy_rslt_t result;
    cy_stc_sysint_t intrCfg =
    {
       /*.intrSrc =*/ CYBSP_TIMER_IRQ, /* Interrupt source is Timer interrupt */
       /*.intrPriority =*/ 3UL   /* Interrupt priority is 3 */
    };

    result = Cy_SysInt_Init(&intrCfg, SelfTest_Interrupt_ISR_TIMER);

    if(result != CY_SYSINT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable Interrupt */
    NVIC_EnableIRQ(intrCfg.intrSrc);


    /* Initialize TCPWM counter*/
    result = Cy_TCPWM_Counter_Init(CYBSP_TIMER_HW, CYBSP_TIMER_NUM, &CYBSP_TIMER_config);
    if(result != CY_TCPWM_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable timer */
    Cy_TCPWM_Counter_Enable(CYBSP_TIMER_HW, CYBSP_TIMER_NUM);

    Cy_TCPWM_SetInterruptMask(CYBSP_TIMER_HW, CYBSP_TIMER_NUM, CY_TCPWM_INT_ON_TC);

}


/*****************************************************************************
* Function Name: clock_test_init
******************************************************************************
* Summary:
* This function initializes the WDT block and initialize the timer interrupt
* for the Self test.
*
* Parameters:
*  void
*
* Return:
*  void
*
*****************************************************************************/
static void clock_test_init(void)
{
    cy_en_tcpwm_status_t tcpwm_res;
    cy_en_sysint_status_t sysint_res;

    /* Write the ignore bits - operate with full 16 bits */
    Cy_WDT_SetIgnoreBits(IGNORE_BITS);

    if(Cy_WDT_GetIgnoreBits() != IGNORE_BITS)
    {
        CY_ASSERT(0);
    }

    /* Clear match event interrupt, if any */
    Cy_WDT_ClearInterrupt();

    /* Enable ILO */
    Cy_SysClk_IloEnable();

    /* Waiting for proper start-up of ILO */
    Cy_SysLib_Delay(ILO_START_UP_TIME);

    Cy_WDT_Enable();
    if(Cy_WDT_IsEnabled() == false)
    {
        CY_ASSERT(0);
    }

    tcpwm_res = Cy_TCPWM_Counter_Init(CYBSP_CLOCK_TEST_TIMER_HW, CYBSP_CLOCK_TEST_TIMER_NUM, &CYBSP_CLOCK_TEST_TIMER_config);
    if(CY_TCPWM_SUCCESS != tcpwm_res)
    {
        CY_ASSERT(0);
    }

    cy_stc_sysint_t intrCfg =
    {
       /*.intrSrc =*/ CYBSP_CLOCK_TEST_TIMER_IRQ, /* Interrupt source is Timer interrupt */
       /*.intrPriority =*/ 3UL   /* Interrupt priority is 3 */
    };

    sysint_res = Cy_SysInt_Init(&intrCfg, SelfTest_Clock_ISR_TIMER);

    if(CY_SYSINT_SUCCESS != sysint_res)
    {
        CY_ASSERT(0);
    }

    /* Enable Interrupt */
    NVIC_EnableIRQ(intrCfg.intrSrc);

    /* Enable timer */
    Cy_TCPWM_Counter_Enable(CYBSP_CLOCK_TEST_TIMER_HW, CYBSP_CLOCK_TEST_TIMER_NUM);

    Cy_TCPWM_SetInterruptMask(CYBSP_CLOCK_TEST_TIMER_HW, CYBSP_CLOCK_TEST_TIMER_NUM, CY_TCPWM_INT_ON_TC);

}

/*****************************************************************************
* Function Name: Start_Up_Test
******************************************************************************
* Summary:
* Start Up Test : This function checks the startup configuration registers.
*
* Parameters:
*  void
*
* Return:
*  void
*****************************************************************************/
static void Start_Up_Test(void)
{
    if(ERROR_STATUS == ret)
    {
        printf("\r\n");
    }
#if COMPONENT_CAT1A
    /* This function initilizes the AREF address depending on the device.*/
    SelfTests_Init_StartUp_ConfigReg();
#endif

#if (STARTUP_CFG_REGS_MODE == CFG_REGS_TO_FLASH_MODE)

    /*******************************/
    /* Save Start-Up registers...  */
    /*******************************/
    if (CY_FLASH_DRV_SUCCESS  != SelfTests_Save_StartUp_ConfigReg())
    {
        /* Process error */
        printf("Error: Can't save Start-Up Config Registers\r\n");
    }

#endif /* End (STARTUP_CFG_REGS_MODE == CFG_REGS_TO_FLASH_MODE) */
    /**********************************/
    /* Run Start-Up regs Self Test... */
    /**********************************/
    ret = SelfTests_StartUp_ConfigReg();

    /* Process error */
    PRINT_TEST_RESULT("Start-Up Register",ret);
    if(ERROR_STATUS == ret)
    {
        printf("\r\n");
    }

}
/* [] END OF FILE */
