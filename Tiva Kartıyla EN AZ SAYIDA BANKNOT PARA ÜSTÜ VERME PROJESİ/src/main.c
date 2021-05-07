#include <stdint.h>
#include "inc/tm4c123gh6pm.h"
#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include <stdbool.h>
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#define BAUDRATE 600

#define RW 0x20 								// PA5
#define RS 0x40  								// PA6
#define E  0x80  								// PA7

volatile unsigned long delay;	// Port aktive ederken işimize yarayacak delay
volatile unsigned long gecikme;

int secim=0;

double para_ustu=0;
int elli=10;
int bes=20;
int on=20;
int yirmi=10;
int yuz=5;
int kopukleme=30;
int cilalama=20;
int kurulama=100;
int yikama=50;
int led;
double atilanPara=0;
double maliyet=0;
char *geciciptr;


/** UART (seri port) ayarini yapan fonksiyon */
void init_UARTstdio() {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	GPIOPinConfigure(0x00000001);
	GPIOPinConfigure(0x00000401);
	GPIOPinTypeUART(0x40004000, 0x00000001 | 0x00000002);
	UARTConfigSetExpClk(0x40004000, SysCtlClockGet(), BAUDRATE,
                        	(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                         	UART_CONFIG_PAR_NONE));
	UARTStdioConfig(0, BAUDRATE, SysCtlClockGet());
}



// LCD komutları:


void portlariAktiflestir(void){
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOA;		// A portunu aktive et
	delay = SYSCTL_RCGC2_R;						// A portunun aktive edilmesini 1 tick bekle
	GPIO_PORTA_AMSEL_R &= ~0b11100000;			// A portunun analog modunu devre dışı bırak
	GPIO_PORTA_PCTL_R &= ~0xFFF00000;			// A portundaki pinlerin voltajını düzenle (PCTL=Power Control)
	GPIO_PORTA_DIR_R |= 0b11100000;				// A portunun giriş çıkışlarını belirle
	GPIO_PORTA_AFSEL_R &= ~0b11100000;			// A portundaki alternatif fonksiyonları seç
	GPIO_PORTA_DEN_R |= 0b11100000;				// A portunun pinlerini aktifleştir
	GPIO_PORTA_DR8R_R |= 0b11100000;			// A portundaki pinlerin 8mA çıkışını aktive et

	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOB;		// B portunu aktive et
	delay = SYSCTL_RCGC2_R;						// B portunun aktive edilmesini 1 tick bekle
	GPIO_PORTB_AMSEL_R &= ~0b11111111;			// B portunun analog modunu devre dışı bırak
	GPIO_PORTB_PCTL_R &= ~0xFFFFFFFF;			// B portundaki pinlerin voltajını düzenle (PCTL=Power Control)
	GPIO_PORTB_DIR_R |= 0b11111111;				// B portunun giriş çıkışlarını belirle
	GPIO_PORTB_AFSEL_R &= ~0b11111111;			// B portundaki alternatif fonksiyonları seç
	GPIO_PORTB_DEN_R |= 0b11111111;				// B portunun pinlerini aktifleştir
	GPIO_PORTB_DR8R_R |= 0b11111111;			// B portundaki pinlerin 8mA çıkışını aktive et

	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOE;		// E portunu aktive et
	delay = SYSCTL_RCGC2_R;						// E portunun aktive edilmesini 1 tick bekle
	GPIO_PORTE_DIR_R |= 0x00;					// E portunun giriş çıkışlarını belirle
	GPIO_PORTE_DEN_R |= 0b00011111;				// E portunun pinlerini aktifleştir

}


void komutGonder(unsigned char LCD_Comment){
	GPIO_PORTA_DATA_R &= ~(RS+RW+E);			// Tüm pinleri sıfırla
	GPIO_PORTB_DATA_R = LCD_Comment;			// Komutu yazdır
	GPIO_PORTA_DATA_R |= E;						// E'yi aç
	GPIO_PORTA_DATA_R &= ~(RS+RW);				// RS ve RW kapat
	for (delay = 0 ; delay < 1; delay++);		// 1us bekle
	GPIO_PORTA_DATA_R &= ~(RS+RW+E);			// RS, RW ve E kapat
	for (delay = 0 ; delay < 1000; delay++);	// 1ms bekle
}

void veriGonder(unsigned char LCD_Data){
	GPIO_PORTB_DATA_R = LCD_Data;				// Write Data
	GPIO_PORTA_DATA_R |= RS+E;					// RS ve E aç
	GPIO_PORTA_DATA_R &= ~RW;					// RW kapat
	for (delay = 0 ; delay < 23 ; delay++);		// 230ns bekle
	GPIO_PORTA_DATA_R &= ~(RS+RW+E);			// RS, RW ve E kapat
	for (delay = 0 ; delay < 1000; delay++);	// 1ms bekle
}

void ekraniAktiflestir(){
	portlariAktiflestir();						// Portları aktifleştir
	for (delay = 0 ; delay < 15000; delay++);	// 15ms bekle
	komutGonder(0x38);							// 0b00111000 -> PortB
	for (delay = 0 ; delay < 5000; delay++);	// 5ms bekle
	komutGonder(0x38);							// 0b00111000 -> PortB
	for (delay = 0 ; delay < 150; delay++);		// 150us bekle
	komutGonder(0x0C);							// 0b00001010 -> PortB
	komutGonder(0x01);							// Ekranı Temizle
	komutGonder(0x06);							// 0b00000110 -> PortB
	for (delay = 0 ; delay < 50000; delay++);	// 50ms bekle
}

void lcdEkran(unsigned int line,unsigned int digit, unsigned char *str){
	unsigned int lineCode = line==1 ?0x80:0xC0;	// Line 1 ise 0x80, 2 ise 0xC0 komutu kullanılması gerekiyor
	komutGonder(lineCode + digit);				// Yazının nereden başlayacağını LCD'ye bildir
	while(*str != 0){ veriGonder(*str++); }		// Ve yazdır
}







int basiliButon(){								// Şu anda basılı olan butonu döndüren fonksiyon
	return  (GPIO_PORTE_DATA_R & 0b00010000) == 0 ? 1
			: (GPIO_PORTE_DATA_R & 0b00100000) == 0 ? 2
					: (GPIO_PORTE_DATA_R & 0b00001000) == 0 ? 3
							: (GPIO_PORTE_DATA_R & 0b00000100) == 0 ? 4
									: (GPIO_PORTE_DATA_R & 0b00000010) == 0 ? 5
											: (GPIO_PORTE_DATA_R & 0b00000001) == 0 ? 6
													: (GPIO_PORTF_DATA_R & 0b00001) == 0 ? 7
															: (GPIO_PORTF_DATA_R & 0b10000) == 0 ? 8
																	: (GPIO_PORTF_DATA_R & 0b00010) == 0 ? 9
			: 0 ;
}




void init_port_E(){

	volatile unsigned long tmp; // bu degisken gecikme yapmak icin gerekli
	   SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOE;   // 1) E portunun osilatörünü etkinleştir
	   tmp = SYSCTL_RCGCGPIO_R;    	// allow time for clock to start
	   GPIO_PORTE_LOCK_R = 0x4C4F434B;   // 2) unlock GPIO Port E
	   GPIO_PORTE_CR_R = 0x3F;         // allow changes to PE5-0 //PE5-0 değişikliklerine izin ver
	                                   // only PE0 needs to be unlocked, other bits can't be locked
	    			 // Sadece PE0 kilidinin açılması gerekir, diğer bitler kilitlenemez
	   GPIO_PORTE_AMSEL_R = 0x00;    	// 3) disable analog on PE //PE'de analog devre dışı bırak
	   GPIO_PORTE_PCTL_R = 0x00000000;   // 4) PCTL GPIO on PE4-0
	   //eski GPIO_PORTE_DIR_R = 0x0F;      	// 5) PE4,PE5 in, PE3-0 out
	   //GPIO_PORTE_DIR_R = 0x07;// 5) PE3,PE4,PE5 in, PE2-0 out
	   //GPIO_PORTE_DIR_R = 0x03;//PE2,PE3,PE4,PE5 in, PE1-0 out
	   //GPIO_PORTE_DIR_R = 0x01;
	   GPIO_PORTE_DIR_R = 0x00;
	   GPIO_PORTE_AFSEL_R = 0x00;    	// 6) disable alt funct on PE7-0

	   //eski GPIO_PORTE_PUR_R = 0x30; //PE4 ve PE5'te pull up'ı etkinleştir ( BUTON İÇİN)// enable pull-up on PE5 and PE4
	   //GPIO_PORTE_PUR_R = 0x38;      	//PE3,PE4 ve PE5'te pull up'ı etkinleştir ( BUTON İÇİN)//
	   //GPIO_PORTE_PUR_R = 0x3c;         //PE2,PE3,PE4 ve PE5'te pull up'ı etkinleştir ( BUTON İÇİN)//
	   //GPIO_PORTE_PUR_R = 0x3E;
	   GPIO_PORTE_PUR_R = 0x3F;
	   GPIO_PORTE_DEN_R = 0x3F;      	// 7) enable digital I/O on PE5-0 // portE 5-0 giriş çıkış  etkinlerştir.

}



void init_portx_F() {
   volatile unsigned long tmp; // bu degisken gecikme yapmak icin gerekli
   tmp = SYSCTL_RCGCGPIO_R;    	// allow time for clock to start
   SYSCTL_RCGCGPIO_R |= 0x00000020;  // 1) activate clock for Port F
   GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock GPIO Port F
   GPIO_PORTF_CR_R = 0x01;       	// allow changes to PF-0
   // only PF0 needs to be unlocked, other bits can't be locked
   //eskiGPIO_PORTF_DIR_R = 0x0E;// 5) PF4,PF0 in, PF3-1 out
   GPIO_PORTF_DIR_R = 0xC;//yeni
   //eskiGPIO_PORTF_PUR_R = 0x11;  // enable pull-up on PF0 and PF4
   GPIO_PORTF_PUR_R = 0x13;//yeni
   GPIO_PORTF_DEN_R = 0x1F;      	// 7) enable digital I/O on PF4-0
}







int main(void) {

	init_port_E();
	ekraniAktiflestir();
	init_portx_F();


init_UARTstdio();







int ikinciasama=0;
ikinciasama=0;
					GPIO_PORTF_DATA_R &= ~(0b00100);
					GPIO_PORTF_DATA_R &= ~(0b01000);
					atilanPara=0;
					maliyet=0;
					para_ustu=0;

while(1){
		if(basiliButon()){										// Eğer herhangi bir butona basılmışsa
			secim = basiliButon();								// Hangi butona basıldığını bir değişkene ata
		}else{													// Buton bırakıldığında (veya hiç basılmadığında)
			if(secim!=0){										// Herhangi bir buton bırakıldıysa
					 if(secim==1){
						 if(ikinciasama==0){
							 komutGonder(0x01);

							 	 	 	 	 atilanPara=atilanPara+5;

						 						char cikti[16] = {((int)atilanPara)/1000+48,(((int)atilanPara)%1000)/100+48,(((int)atilanPara)%100)/10+48,((int)atilanPara)%10+48,' ', 'T','L',' ','a','t','i','l','d','i','\0'};

						 						lcdEkran(1,0,cikti);


						 }
						 if(ikinciasama==1){
							 komutGonder(0x01);

							 maliyet=maliyet+15;

							 char cikti2[10]={((int)maliyet)/1000+48,(((int)maliyet)%1000)/100+48,(((int)maliyet)%100)/10+48,((int)maliyet)%10+48,' ','t','l','\0' };
							 lcdEkran(1,0,cikti2);
							 lcdEkran(2,0,"Kopukleme 15 TL");
							 kopukleme--;


						 }







					 }
				else if(secim==2){
					if(ikinciasama==0){

						komutGonder(0x01);

						atilanPara=atilanPara+10;
						char cikti[16] = {((int)atilanPara)/1000+48,(((int)atilanPara)%1000)/100+48,(((int)atilanPara)%100)/10+48,((int)atilanPara)%10+48,' ', 'T','L',' ','a','t','i','l','d','i','\0'};


						lcdEkran(1,0,cikti);

					}
					if(ikinciasama==1){
						 komutGonder(0x01);

						 maliyet=maliyet+10;

						 char cikti2[10]={((int)maliyet)/1000+48,(((int)maliyet)%1000)/100+48,(((int)maliyet)%100)/10+48,((int)maliyet)%10+48,' ','t','l','\0' };
						 lcdEkran(1,0,cikti2);
													 lcdEkran(2,0,"Yikama 10 TL");
													 yikama--;

					}


				}

				else if(secim==3){
					if(ikinciasama==0){
						komutGonder(0x01);

						atilanPara=atilanPara+20;


						char cikti[16] = {((int)atilanPara)/1000+48,(((int)atilanPara)%1000)/100+48,(((int)atilanPara)%100)/10+48,((int)atilanPara)%10+48,' ', 'T','L',' ','a','t','i','l','d','i','\0'};
						lcdEkran(1,0,cikti);

					}
					if(ikinciasama==1){
						 komutGonder(0x01);

						 maliyet=maliyet+5;

						 char cikti2[7]={((int)maliyet)/1000+48,(((int)maliyet)%1000)/100+48,(((int)maliyet)%100)/10+48,((int)maliyet)%10+48,' ','t','l','\0' };
						 lcdEkran(1,0,cikti2);
													 lcdEkran(2,0,"Kurulama 5 TL");
													 kurulama--;

					}

								}
				else if(secim==4){
					if(ikinciasama==0){
					 komutGonder(0x01);

					 	 	 	 	 atilanPara=atilanPara+50;

					 	 	 		char cikti[16] = {((int)atilanPara)/1000+48,(((int)atilanPara)%1000)/100+48,(((int)atilanPara)%100)/10+48,((int)atilanPara)%10+48,' ', 'T','L',' ','a','t','i','l','d','i','\0'};
					 	 	 		lcdEkran(1,0,cikti);


				 }

					if(ikinciasama==1){
											 komutGonder(0x01);

											 maliyet=maliyet+50;

											 char cikti2[7]={((int)maliyet)/1000+48,(((int)maliyet)%1000)/100+48,(((int)maliyet)%100)/10+48,((int)maliyet)%10+48,' ','t','l','\0' };
											 lcdEkran(1,0,cikti2);
																		 lcdEkran(2,0,"Cilalama 50 TL");
                                                                         cilalama--;

										}


												}
				else if(secim==5){
					if(ikinciasama==0){
										komutGonder(0x01);

										atilanPara=atilanPara+100;

										char cikti[16] = {((int)atilanPara)/1000+48,(((int)atilanPara)%1000)/100+48,(((int)atilanPara)%100)/10+48,((int)atilanPara)%10+48,' ', 'T','L',' ','a','t','i','l','d','i','\0'};
										lcdEkran(1,0,cikti);

									}


																}
				else if(secim==6){
					//asama degistirme butonu
					komutGonder(0x01);

					ikinciasama=1;
					lcdEkran(1,0,"Hizmet Seciniz");
				}
				else if(secim==7){
					//para takilmasi oldugunda kirmizi led yanar.


				//	int paraTakilma=rand()%4+1;
					komutGonder(0x01);
					//if(paraTakilma==2){
						//lcdEkran(1,0,"Para Takildi");

						//GPIO_PORTF_DATA_R |= 0b01000;

//}else{
						para_ustu=atilanPara-maliyet;
if(para_ustu>=0){

	                                        int banknotKontrol=0;
	                                     	int kalan=para_ustu;
	                                     	int toplam=yuz*100+elli*50+yirmi*20+on*10+bes*5;
											if(yuz>0 && toplam>=para_ustu)
											{
											yuz=yuz-kalan/100;
											kalan=kalan%100;
											}
											if(elli>0 && toplam>=para_ustu)
											{
											elli=elli-kalan/50;
											kalan=kalan%50;

											}
											if(yirmi>0 && toplam>=para_ustu)
											{
											yirmi=yirmi-kalan/20;
											kalan=kalan%20;

											}
											 if(on>0 && toplam>=para_ustu)
											{
											on=on-kalan/10;
											kalan=kalan%10;

											}
										    if(bes>0 && toplam>=para_ustu)
											{
												bes=bes-kalan/5;
											}
											else
											{

												banknotKontrol=-1;
											}
											if(banknotKontrol==-1)
											{
												lcdEkran(2,0,"banknot bitti");

											}
											else
											{
					 char paraUstuCikti[16]={((int)para_ustu)/1000+48,(((int)para_ustu)%1000)/100+48,(((int)para_ustu)%100)/10+48,((int)para_ustu)%10+48,' ','t','l',' ','P','a','r','a',' ','U','s','t','u' };
					 char kasa[16]={((int)bes)/10+48,((int)bes)%10+48,' ',((int)on)/10+48,((int)on)%10+48,' ',((int)yirmi)/10+48,((int)yirmi)%10+48,' ',((int)elli)/10+48,((int)elli)%10+48,' ',((int)yuz)/10+48,((int)yuz)%10+48};
																	 lcdEkran(1,0,paraUstuCikti);
																						lcdEkran(2,0,kasa);
																						GPIO_PORTF_DATA_R |= 0b00100;
																						//GPIO_PORTF_DATA_R &= ~(0b01000);
																						//yesil yak
											}




}
else{

	lcdEkran(2,0,"para yetersiz");
}



//}





				}
				else if(secim==8){
					komutGonder(0x01);
					ikinciasama=0;
					GPIO_PORTF_DATA_R &= ~(0b00100);
					GPIO_PORTF_DATA_R &= ~(0b01000);
					atilanPara=0;
					maliyet=0;
					para_ustu=0;

				    elli=10;
					bes=20;
				    on=20;
				    yirmi=10;
					yuz=5;
					kopukleme=30;
					cilalama=20;
					kurulama=100;
					yikama=50;


					lcdEkran(1,0,"Resetlendi");
				}
				else if(secim==9){
					//basa donme
					komutGonder(0x01);
					ikinciasama=0;
					atilanPara=0;
					maliyet=0;
					para_ustu=0;


					GPIO_PORTF_DATA_R &= ~(0b00100);
					lcdEkran(1,0,"Basa Don");
				}




			}
			secim=0;											// Seçimi sıfırla
		}

	    for (delay = 0 ; delay < 1000 ; delay++);
	}

	}





