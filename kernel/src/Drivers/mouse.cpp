#include <Drivers/mouse.hpp>
#include <kernel/sys/IOPort.hpp>
#include <kernel/sys/handlers.hpp>
#include <hardware/pic8259.hpp>
#include <stdio.h>

using namespace IOPort;

static void mouse_waitcmd(unsigned char type);

static void mouse_write (unsigned char a_write);
static unsigned char mouse_read();

static void mouse_dead();
static void mouse_IRQhandler(void* arg);

void mouse_init()
{
	printf("mouse_init");
	mouse_waitcmd(1);
	outportb(0x64,0xA8);
	mouse_waitcmd(1);
	outportb(0x64,0x20);
	unsigned char status_byte;
	mouse_waitcmd(0);
	status_byte = (inportb(0x60) | 2);
	mouse_waitcmd(1);
	outportb(0x64, 0x60);
	mouse_waitcmd(1);
	outportb(0x60, status_byte);
	mouse_write (0xF6);
	mouse_read();
	mouse_write (0xF4);
	mouse_read();
    Handlers::AddInterruptHandler(32, (interrupt_handle_function)mouse_IRQhandler, nullptr);
    PIC8259::irq_enable(12);
}

/**
  * Disable the mouse driver
  * @author Ivan Gualandri, Diego Stamigni, DT
  * @version 1.0
  */
void mouse_dead()
{
	printf("mouse_dead");
    PIC8259::irq_disable(12);
	mouse_write(0xF5); 
	mouse_read();
}

/**
 * Mouse wait for a command
 * @param type: 1 for sending - 0 for receive
 * @return none
 **/
void mouse_waitcmd(unsigned char type)
{
	register unsigned int _time_out=100000;
	if(type==0)
	{
		while(_time_out--) // dati
		{
			if((inportb(0x64) & 1)==1)
			{
				return;
			}
		}
		return;
	}
	else
	{
		while(_time_out--) // e segnali
		{
			if((inportb(0x64) & 2)==0)
			{
				return;
			}
		}
		return;
	}
}

/**
 * Send data to mouse
 * @param data data to send
 **/
void mouse_write (unsigned char a_write)
{
	mouse_waitcmd(1);
	outportb(0x64, 0xD4);
	mouse_waitcmd(1);
	outportb(0x60, a_write);
}

/**
 * Read data from mouse
 * @return data received from mouse
 **/
unsigned char mouse_read()
{
	mouse_waitcmd(0);
	return inportb(0x60);
}

void mouse_IRQhandler(void* arg)
{
	static unsigned char cycle = 0;
	static char mouse_bytes[3];
	mouse_bytes[cycle++] = inportb(0x60);
	// dichiariamo le due variabili che conterranno le coordinate
	// e inizializziamole a 0
	signed long int MousePositionX = 0, MousePositionY = 0;
      
	// e alliniamolo al centro!
	// non va bene _SCR_H e _SCR_W
	//MousePositionX=(_SCR_H/2)-(_SCR_W/2);
	//MousePositionY=(_SCR_H/2)-(_SCR_W/2);
	MousePositionX=(800/2)-(800/2);
	MousePositionY=(800/2)-(600/2);

	if (cycle == 3) {
	  cycle = 0; 

	//if ((mouse_bytes[0] & 0x80) || (mouse_bytes[0] & 0x40))
	//  return;

	// coordinate x stanno in mouse_bytes[1] e
	// le coordinate y stanno in mouse_bytes[2]
	// direzione (0=destra, 1=sinitra)

	//if((mouse_bytes[0] & 0x07)==0) // 0x07 forse non � un valore corretto
	if((mouse_bytes[0] & 0x40)==0)
	{
	  //if((mouse_bytes[0] & 0x05)==0) // 0x05 forse non � un valore corretto
	  if((mouse_bytes[0] & 0x10)==0)
	  {
	      MousePositionX+=mouse_bytes[1];
	  } else {
	      MousePositionX-=mouse_bytes[1];
	  }
	} else //overflow
	  MousePositionX+=mouse_bytes[1]/2;
   
	// direzioni (0=su, 1=sotto)
	//if((mouse_bytes[0] & 0x08)==0) // 0x08 forse non � un valore corretto
	if((mouse_bytes[0] & 0x80)==0)
	{
	  //if((mouse_bytes[0] & 0x06)==0) // 0x06 forse non � un valore corretto
	  if((mouse_bytes[0] & 0x20)==0)
	  {
	      MousePositionY-=mouse_bytes[2];
	  } else {
	      MousePositionY+=mouse_bytes[2];
	  }
	} else  // overflow
	    MousePositionY-=mouse_bytes[2]/2;
	
	// manteniamo il cursore entro i limiti 
	// per uno schermo 800x600
	// per MousePositionX
	if(MousePositionX>=800-16) 
	  MousePositionX=800-16;

	if(MousePositionX<=0)
	  MousePositionX=0;

	// e per MousePositionY
	if(MousePositionY>=600-24)
	  MousePositionY=600-24;

	if(MousePositionY<=0)
	  MousePositionY=0;

	// stampa la posizione
	printf("\rx: %d | y: %d", MousePositionX, MousePositionY);

	// e muove il cursore 
	//_ksetcursor(MousePositionX, MousePositionY); // Not now

	// Qui � si rilevato un problema, se il mouse si muove
	// vengono rilevati tasti premuti..
/*
	// Rilevo i tasti premuti..
	if (mouse_bytes[0] & 0x4) 
	    printf(LNG_MOUSE_MID);  // Centrale premuto

	if (mouse_bytes[0] & 0x2) 
	    printf(LNG_MOUSE_RIGHT);  // Destro premuto

	if (mouse_bytes[0] & 0x1) 
	    printf(LNG_MOUSE_LEFT);  // Sinistro premuto
*/
	}

} //End