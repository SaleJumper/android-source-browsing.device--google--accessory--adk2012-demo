/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support
 * ----------------------------------------------------------------------------
 * Copyright (c) 2011, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

/**
 * \file
 *
 * Implementation of ADS7843 driver.
 *
 */

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "board.h"

/*----------------------------------------------------------------------------
 *        Definitions
 *----------------------------------------------------------------------------*/

#define ADS_CTRL_PD0              (1 << 0)        /* PD0 */
#define ADS_CTRL_PD1              (1 << 1)        /* PD1 */
#define ADS_CTRL_DFR              (1 << 2)        /* SER/DFR */
#define ADS_CTRL_EIGHT_BITS_MOD   (1 << 3)        /* Mode */
#define ADS_CTRL_START            (1 << 7)        /* Start Bit */
#define ADS_CTRL_SWITCH_SHIFT     4               /* Address setting */

/* Get X position command */
#define CMD_X_POSITION ((1 << ADS_CTRL_SWITCH_SHIFT) | ADS_CTRL_START | ADS_CTRL_PD0 | ADS_CTRL_PD1)
/* Get Y position command */
#define CMD_Y_POSITION ((5 << ADS_CTRL_SWITCH_SHIFT) | ADS_CTRL_START | ADS_CTRL_PD0 | ADS_CTRL_PD1)

/* Enable penIRQ */
#define CMD_ENABLE_PENIRQ  ((1 << ADS_CTRL_SWITCH_SHIFT) | ADS_CTRL_START)

#define ADS7843_TIMEOUT        5000000

#define DELAY_BEFORE_SPCK          200 /* 2us min (tCSS) <=> 200/100 000 000 = 2us */
#define DELAY_BETWEEN_CONS_COM     0xf /* 5us min (tCSH) <=> (32 * 15) / (100 000 000) = 5us */

/*----------------------------------------------------------------------------
 *        Local variables
 *----------------------------------------------------------------------------*/

/** Pins used by SPI */
static const Pin pinsSPI[]  = { BOARD_TSC_SPI_PINS, BOARD_TSC_NPCS_PIN } ;

/** Touch screen BUSY pin */
static const Pin pinBusy[] = {PIN_TSC_BUSY};

/** Touch screen NPCS pin */
static const Pin pinCs[] = {BOARD_TSC_NPCS_PIN};

/*----------------------------------------------------------------------------
 *        Local functions
 *----------------------------------------------------------------------------*/

/**
 * \brief Generic function to send a command to the touchscreen controller.
 *
 * \param bCmd command to send
 *
 * \return command result
 */
static uint32_t SendCommand( uint8_t ucCmd )
{
    uint32_t  dwResult = 0 ;
    uint32_t dw ;
    Spi* pSpi ;

    /* (volatile declaration needed for code optimisation by compiler) */
    volatile uint8_t bufferRX[3] ;
    volatile uint8_t bufferTX[3] ;

    pSpi = BOARD_TSC_SPI_BASE ;
    bufferRX[0] = 0 ;
    bufferRX[1] = 0 ;
    bufferRX[2] = 0 ;

    bufferTX[0] = ucCmd ;
    bufferTX[1] = 0 ;
    bufferTX[2] = 0 ;

    /* Send Command and data through the SPI */
    PIO_PinClear( pinCs ) ;
    // Send command
    dw = 0 ;
    while ( (pSpi->SPI_SR & SPI_SR_TXEMPTY) == 0 ) ;
    pSpi->SPI_TDR = bufferTX[dw] | SPI_PCS( BOARD_TSC_NPCS ) ;
    while ( (pSpi->SPI_SR & SPI_SR_TDRE) == 0 ) ;

    while ( PIO_PinGet( pinBusy ) == 1 ) ;
    while ( (pSpi->SPI_SR & SPI_SR_RDRF) == 0 ) ;
    bufferRX[dw] = pSpi->SPI_RDR & 0xFFFF ;

    // Read data
    for ( dw = 1 ; dw < 3 ; dw++ )
    {
        while ( (pSpi->SPI_SR & SPI_SR_TXEMPTY) == 0 ) ;
        pSpi->SPI_TDR = bufferTX[dw] | SPI_PCS( BOARD_TSC_NPCS ) ;
        while ( (pSpi->SPI_SR & SPI_SR_TDRE) == 0);

        while ( (pSpi->SPI_SR & SPI_SR_RDRF) == 0 ) ;
        bufferRX[dw] = pSpi->SPI_RDR & 0xFFFF ;
    }
    PIO_PinSet( pinCs ) ;

    dwResult = (uint32_t)bufferRX[1] << 8 ;
    dwResult |= (uint32_t)bufferRX[2] ;
    dwResult = dwResult >> 4 ;

    return dwResult ;
}

/*----------------------------------------------------------------------------
 *        Exported functions
 *----------------------------------------------------------------------------*/

/**
 * \brief Get position of the pen by ask the ADS controller (SPI).
 *
 * \param px_pos pointer to the horizontal position
 * \param py_pos pointer to the vertical position
 *
 */
extern void ADS7843_GetPosition( uint32_t *px_pos, uint32_t *py_pos )
{
    /* Get X position */
    *px_pos = SendCommand( CMD_X_POSITION ) ;
    /* Get Y position */
    *py_pos = SendCommand( CMD_Y_POSITION ) ;
    /* Switch to full power mode */
    SendCommand( CMD_ENABLE_PENIRQ ) ;
}

/**
 * \brief Initialization of the SPI for communication with ADS7843 component.
 */
extern void ADS7843_Initialize( void )
{
    volatile uint32_t dwDummy ;

    /* Configure pins */
    PIO_PinConfigure( pinsSPI, PIO_LISTSIZE( pinsSPI ) ) ;

    PIO_PinConfigure( pinBusy, PIO_LISTSIZE( pinBusy ) ) ;

    SPI_Configure( BOARD_TSC_SPI_BASE, BOARD_TSC_SPI_ID,
                  SPI_MR_MSTR | SPI_MR_MODFDIS | SPI_PCS(BOARD_TSC_NPCS) /* Value of the SPI configuration register. */
    ) ;

    SPI_ConfigureNPCS( BOARD_TSC_SPI_BASE, BOARD_TSC_NPCS,
                       SPI_CSR_NCPHA | SPI_CSR_DLYBS(DELAY_BEFORE_SPCK) |
                       SPI_CSR_DLYBCT(DELAY_BETWEEN_CONS_COM) | SPI_CSR_SCBR(0xC8) );

    SPI_Enable( BOARD_TSC_SPI_BASE ) ;

    for ( dwDummy=0 ; dwDummy < 100000 ; dwDummy++ ) ;

    dwDummy = BOARD_TSC_SPI_BASE->SPI_SR ;
    dwDummy = BOARD_TSC_SPI_BASE->SPI_RDR ;

    SendCommand( CMD_ENABLE_PENIRQ ) ;
}

/**
 * \brief Reset the ADS7843
 */
void ADS7843_Reset( void )
{
    /* Disable SPI */
    SPI_Disable( BOARD_TSC_SPI_BASE ) ;
}

