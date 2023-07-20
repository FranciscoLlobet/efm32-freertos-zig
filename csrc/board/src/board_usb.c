#include "board.h"

USBX_STRING_DESC(usb_product_string, 'X','D','K',' ','A','p','p','l','i','c','a','t','i','o','n');
USBX_STRING_DESC(usb_manufacturer_string, 'm','i','s','o');
USBX_STRING_DESC(usb_serial_string, '0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0');

USBX_Init_t usbx;

USBX_BUF(usb_rx_buf, 64);
USBX_BUF(usb_tx_buf, 64);

void BOARD_USB_Callback(void)
{
	uint32_t intSource = USBX_getCallbackSource();
	if(intSource & USBX_RESET)
	{
		// USB Reset
	}
	if(intSource & USBX_TX_COMPLETE)
	{

	}
	if(intSource & USBX_DEV_OPEN)
	{

	}
	if(intSource & USBX_DEV_CLOSE)
	{

	}
	if(intSource & USBX_DEV_CONFIGURED)
	{

	}
	if(intSource & USBX_DEV_SUSPEND)
	{

	}
	if(intSource & USBX_RX_OVERRUN)
	{

	} 
}


void BOARD_USB_Init(void)
{
	usbx.serialString= (void*)&usb_serial_string;
	usbx.productString= (void *)&usb_product_string;
	usbx.manufacturerString =(void*)&usb_manufacturer_string;
	usbx.vendorId = (uint16_t)0x108C;
	usbx.productId = (uint16_t)0x017B;
	usbx.maxPower = (uint8_t)0xFA;
	usbx.powerAttribute = (uint8_t)0x80;
	usbx.releaseBcd = (uint16_t)0x01;
	usbx.useFifo = 0;
	//USBX_init(&usbx);
}