///////////////////////////////////// BUTTON DEBOUNCING FUNCTION /////////////////////////////////////////////////////////////////////
#pragma interrupt InterruptHandler_2534 ipl1 vector 0
void InterruptHandler_2534( void )
{
   if( INTGetFlag(INT_T2) )             // Verify source of interrupt
   {
      Button1History <<= 1; // Discard oldest sample to make room for new
      if(PORTG & 0x40) // Record the latest BTN1 sample
      Button1History |= 0x01;
      Button2History <<= 1; // Discard oldest sample to make room for new
      if(PORTG & 0x80) // Record the latest BTN2 sample
      Button2History |= 0x01;
      Button3History <<= 1; // Discard oldest sample to make room for new
      if(PORTA & 0x1) // Record the latest BTN2 sample
      Button3History |= 0x01;

      INTClearFlag(INT_T2);             // Acknowledge interrupt
   }
   return;
}
    



    while (1) 
    {
        sprintf(OLEDout1, "L/R=%4d ",cur_LR);
        OledSetCursor(0, 0);
        OledPutString(OLEDout1);
        sprintf(OLEDout1, "U/D=%4d", cur_UD);
        OledSetCursor(0, 1);
        OledPutString(OLEDout1);

        if(PORTG & 0x40)
        {
            LATGSET = 1 << 12;

        }
        else if (PORTG & 0x80)
        {
            LATGSET = 1 << 13;
        }
        else if (PORTA & 0x1)
        {
            LATGSET = 1 << 14;
        }
        else
        {
            LATGCLR = 1 << 12;   // LED1 is cleared/off
             LATGCLR = 1 << 13;   // LED2 is cleared/oof
             LATGCLR = 1 << 14;   // LED3 is cleared/off
             LATGCLR = 1 << 15;   // LED4 is cleared/off

        }
        if (cur_LR >= 600)
        {
            OledSetCursor(9, 2);
            sprintf(OLEDout1, "RIGHT");
            OledPutString(OLEDout1);
        }
        else
        {
            OledSetCursor(9, 2);
            sprintf(OLEDout1, "      ");
            OledPutString(OLEDout1);
        }

        if (cur_LR <= 460)
        {
            OledSetCursor(0, 2);
            sprintf(OLEDout1, "LEFT");
            OledPutString(OLEDout1);
        }
        else
        {
            OledSetCursor(0, 2);
            sprintf(OLEDout1, "      ");
            OledPutString(OLEDout1);
        }

        if (cur_UD >= 600)
        {
            OledSetCursor(0, 3);
            sprintf(OLEDout1, "UP");
            OledPutString(OLEDout1);
        }
        else
        {
            OledSetCursor(0, 3);
            sprintf(OLEDout1, "      ");
            OledPutString(OLEDout1);
        }

        if (cur_UD <= 460)
        {
            OledSetCursor(9, 3);
            sprintf(OLEDout1, "DOWN");
            OledPutString(OLEDout1);
        }
        else
        {
            OledSetCursor(9, 3);
            sprintf(OLEDout1, "      ");
            OledPutString(OLEDout1);
        }
        OledUpdate();
        INTClearFlag(INT_AD1);

        
    }
    return (EXIT_SUCCESS);
}

void START_INITIAL()
{
    
    // Initialize GPIO for LEDs
    DDPCONbits.JTAGEN = 0; // JTAG controller disable to use BTN3

    TRISGSET = 0x40;     // For BTN1: configure PortG bit for input
    TRISGSET = 0x80;     // For BTN2: configure PortG bit for input
    TRISASET = 0x1;

    TRISGCLR = 0xf000;   // For LEDs: configure PortG pins for output
    ODCGCLR  = 0xf000;   // For LEDs: configure as normal output (not open drain)
    LATGCLR  = 0xf000;   // Initialize LEDs to 0000

    TRISBCLR = 0x0040; // initialize U/D pin B6
    ODCBCLR = 0x0040;
    LATBSET = 0x0040;

    // Initialize Timer1 and OLED for display
    DelayInit();
    OledInit();
 ///////////////////////////////////////// ADC ENABLING ///////////////////////////////////////////////////////

    SetChanADC10(AD_MUX_CONFIG);
    OpenADC10(AD_CONFIG1, AD_CONFIG2, AD_CONFIG3, AD_CONFIGPORT, AD_CONFIGSCAN);
    EnableADC10();
    // Set up, clear, and enable ADC interrupts
    INTSetVectorPriority(INT_ADC_VECTOR, INT_PRIORITY_LEVEL_7);
    INTClearFlag(INT_AD1);
    INTEnable(INT_AD1, INT_ENABLED);
/////////////////////////////////////////TIMERS ENABLING ///////////////////////////////////////////////////////

    OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_16 | T2_GATE_OFF, 624);
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
    INTClearFlag(INT_T2);
    INTEnable(INT_T2, INT_ENABLED);

///////////////////////////////// MULTIPLE TIMER INTERRUPT DECLARATION//////////////////////////////
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR); // Configure the system for vectored interrupts
    INTEnableInterrupts();
    OledClearBuffer();
    OledUpdate();

}

                /*
                if ((s_position[0].Xpos == s_position[1].Xpos) && (s_position[0].Ypos == s_position[1].Ypos))
                {
                    cur_var = OVER;
                    break;
                }
                if ((s_position[0].Xpos == s_position[2].Xpos) && (s_position[0].Ypos == s_position[2].Ypos))
                {
                    cur_var = OVER;
                    break;
                }
                if ((s_position[0].Xpos == s_position[3].Xpos) && (s_position[0].Ypos == s_position[3].Ypos))
                {
                    cur_var = OVER;
                    break;
                }
                if ((s_position[0].Xpos == s_position[4].Xpos) && (s_position[0].Ypos == s_position[4].Ypos))
                {
                    cur_var = OVER;
                    break;
                }
                if ((s_position[0].Xpos == s_position[5].Xpos) && (s_position[0].Ypos == s_position[5].Ypos))
                {
                    cur_var = OVER;
                    break;
                }
                if ((s_position[0].Xpos == s_position[6].Xpos) && (s_position[0].Ypos == s_position[6].Ypos))
                {
                    cur_var = OVER;
                    break;
                }

                int j = size - 1;
                while(j > 0)
                {
                    if ((s_position[0].Xpos == s_position[j].Xpos) && (s_position[0].Ypos == s_position[j].Ypos))
                    {
                        cur_var = OVER;
                        break;
                    }
                    j--;
                }
                */

