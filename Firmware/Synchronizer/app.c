#include "hwbp_core.h"
#include "hwbp_core_regs.h"
#include "hwbp_core_types.h"

#include "app.h"
#include "app_ios_and_regs.h"


/************************************************************************/
/* Declare application registers                                        */
/************************************************************************/
extern AppRegs app_regs;
extern uint8_t app_regs_type[];
extern uint16_t app_regs_n_elements[];
extern uint8_t *app_regs_pointer[];
extern void (*app_func_rd_pointer[])(void);
extern bool (*app_func_wr_pointer[])(void*);


/************************************************************************/
/* Initialize app                                                       */
/************************************************************************/
static const uint8_t default_device_name[] = "Synchronizer";

void hwbp_app_initialize(void)
{	
    /* Define versions */
    uint8_t hwH = 1;
    uint8_t hwL;
    uint8_t fwH = 1;
    uint8_t fwL = 8;
    uint8_t ass = 0;
    
    io_pin2in(&PORTB, 3, PULL_IO_UP, SENSE_IO_EDGES_BOTH);
    
    if (read_io(PORTB, 3))
    {
       hwL = 0;
    }
    else
    {
       hwL = 1;
    }
    
   	/* Start core */
   	core_func_start_core(
   	    1104,
   	    hwH, hwL,
   	    fwH, fwL,
   	    ass,
   	    (uint8_t*)(&app_regs),
   	    APP_NBYTES_OF_REG_BANK,
   	    APP_REGS_ADD_MAX - APP_REGS_ADD_MIN + 1,
   	    default_device_name,
   	    false,	// The device is _not_ able to repeat the harp timestamp clock
   	    false,	// The device is _not_ able to generate the harp timestamp clock
   	    0		// Default timestamp offset
   	);
}

/************************************************************************/
/* Handle if a catastrophic error occur                                 */
/************************************************************************/
void core_callback_catastrophic_error_detected(void)
{
	clr_LEDIN0;
	clr_LEDIN1;
	clr_LEDIN2;
	clr_LEDIN3;
	clr_LEDIN4;
	clr_LEDIN5;
	clr_LEDIN6;
	clr_LEDIN7;
	clr_LEDIN8;

	clr_LEDOUT0;
	clr_OUTPUT0;
}

/************************************************************************/
/* General definitions                                                  */
/************************************************************************/

/************************************************************************/
/* General used functions                                               */
/************************************************************************/

/************************************************************************/
/* Initialization Callbacks                                             */
/************************************************************************/

void core_callback_define_clock_default(void) {}

void core_callback_initialize_hardware(void)
{
	/* Initialize IOs */
	/* Don't delete this function!!! */
	init_ios();
}

void core_callback_reset_registers(void)
{
	/* Initialize registers */
	app_regs.REG_OUTPUTS = 0;
	app_regs.REG_INPUT_CATCH_MODE = GM_INMODE_WHEN_ANY_CHANGE;
	app_regs.REG_OUTPUT_MODE = GM_OUTMODE_TOGGLE;
	app_regs.REG_EVNT_ENABLE = B_EVT0;
}

void core_callback_registers_were_reinitialized(void)
{
	/* Update registers, output 0 and output LED */
	app_regs.REG_INPUTS_STATE = ((~PORTA_IN) & 0x3F) | (((~PORTB_IN) & 0x7) << 6) | (PORTC_IN & 0x01 ? 0x2000 : 0) | (PORTA_IN & 0x80 ? 0x4000 : 0) | (PORTC_IN & 0x02 ? 0x8000 : 0);
	if ((app_regs.REG_OUTPUT_MODE & MSK_OUTPUT_MODE) == GM_OUTMODE_INPUT0)
	{
		if (!read_INPUT0)
		{
			app_regs.REG_OUTPUTS = B_OUTPUT0;
			if (core_bool_is_visual_enabled())
			{
				set_OUTPUT0;
				set_LEDOUT0;
			}
		}
		else
		{
			app_regs.REG_OUTPUTS = 0;
			clr_OUTPUT0;
			clr_LEDOUT0;
		}			
	}

	/* Update LEDs */
	if (core_bool_is_visual_enabled())
	{
		PORTD_OUT = (PORTA_IN & 0x3F);
		PORTC_OUT = (PORTC_OUT & 0x8F) | ((PORTB_IN & 0x7) << 4);
	}
}

/************************************************************************/
/* Callbacks: Visualization                                             */
/************************************************************************/
void core_callback_visualen_to_on(void)
{
	PORTD_OUT = (PORTA_IN & 0x3F);
	PORTC_OUT = (PORTC_OUT & 0x8F) | ((PORTB_IN & 0x7) << 4);

	if (read_OUTPUT0)
		set_LEDOUT0;
	else
		clr_LEDOUT0;
}

void core_callback_visualen_to_off(void)
{
	PORTD_OUT |= 0x3F;
	PORTB_OUT |= 0x07;
	clr_LEDOUT0;
}

/************************************************************************/
/* Callbacks: Change on the operation mode                              */
/************************************************************************/
void core_callback_device_to_standby(void) {}
void core_callback_device_to_active(void) {}
void core_callback_device_to_enchanced_active(void) {}
void core_callback_device_to_speed(void) {}

/************************************************************************/
/* Callbacks: 1 ms timer                                                */
/************************************************************************/
extern void read(bool filter_equal_readings);
uint16_t catch_counter = 0;
void core_callback_t_before_exec(void)
{
    if ((app_regs.REG_INPUT_CATCH_MODE & MSK_CATCH_MODE) >= GM_INMODE_100Hz)
	{
		switch (app_regs.REG_INPUT_CATCH_MODE & MSK_CATCH_MODE)
		{
			case GM_INMODE_100Hz:
				if ((catch_counter++ % 20) == 0)
					read(false);
				break;
								
			case GM_INMODE_250Hz:
				if ((catch_counter++ % 8) == 0)
					read(false);
				break;
				
			case GM_INMODE_500Hz:
				if ((catch_counter++ % 4) == 0)
					read(false);
				break;
				
			case GM_INMODE_1000Hz:
				if ((catch_counter++ % 2) == 0)
					read(false);
				break;
				
			case GM_INMODE_2000Hz:
				read(false);
				break;
		}
	}
}
void core_callback_t_after_exec(void) {}
void core_callback_t_new_second(void)
{
	catch_counter = 0;
}
void core_callback_t_500us(void) {}
void core_callback_t_1ms(void) {}

/************************************************************************/
/* Callbacks: clock control                                             */
/************************************************************************/
void core_callback_clock_to_repeater(void) {}
void core_callback_clock_to_generator(void) {}
void core_callback_clock_to_unlock(void) {}
void core_callback_clock_to_lock(void) {}

/************************************************************************/
/* Callbacks: uart control                                              */
/************************************************************************/
void core_callback_uart_rx_before_exec(void) {}
void core_callback_uart_rx_after_exec(void) {}
void core_callback_uart_tx_before_exec(void) {}
void core_callback_uart_tx_after_exec(void) {}
void core_callback_uart_cts_before_exec(void) {}
void core_callback_uart_cts_after_exec(void) {}

/************************************************************************/
/* Callbacks: TCD1 overflow                                             */
/************************************************************************/
void core_callback_tcd1_overflow(void) {}

/************************************************************************/
/* Callbacks: Read app register                                         */
/************************************************************************/
bool core_read_app_register(uint8_t add, uint8_t type)
{
	/* Check if it will not access forbidden memory */
	if (add < APP_REGS_ADD_MIN || add > APP_REGS_ADD_MAX)
		return false;
	
	/* Check if type matches */
	if (app_regs_type[add-APP_REGS_ADD_MIN] != type)
		return false;
	
	/* Receive data */
	(*app_func_rd_pointer[add-APP_REGS_ADD_MIN])();	

	/* Return success */
	return true;
}

/************************************************************************/
/* Callbacks: Write app register                                        */
/************************************************************************/
bool core_write_app_register(uint8_t add, uint8_t type, uint8_t * content, uint16_t n_elements)
{
	/* Check if it will not access forbidden memory */
	if (add < APP_REGS_ADD_MIN || add > APP_REGS_ADD_MAX)
		return false;
	
	/* Check if type matches */
	if (app_regs_type[add-APP_REGS_ADD_MIN] != type)
		return false;

	/* Check if the number of elements matches */
	if (app_regs_n_elements[add-APP_REGS_ADD_MIN] != n_elements)
		return false;

	/* Process data and return false if write is not allowed or contains errors */
	return (*app_func_wr_pointer[add-APP_REGS_ADD_MIN])(content);
}