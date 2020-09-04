#include "stm32f4xx.h"
#include "defines.h"
#include "tm_stm32f4_delay.h"
#include "tm_stm32f4_ili9341_ltdc.h"
#include "tm_stm32f4_adc.h"
#include "tm_stm32f4_disco.h"
#include "tm_stm32f4_sdram.h"
#include "tm_stm32f4_dac_signal.h"
#include "tm_stm32f4_dac.h"
#include <stdio.h>
#include "arm_math.h"

/* FFT settings */
#define SAMPLES					256			 
#define FFT_SIZE				SAMPLES / 2		
#define N		99                    /* Filter lenght */
#define FFT_BAR_MAX_HEIGHT		140 			

GPIO_InitTypeDef GPIO_InitLEDDIE;


float32_t Input[SAMPLES];
float32_t OutputLOW[FFT_SIZE+N];
float32_t Output[FFT_SIZE];
float32_t  hn[N]={0.00016019,0.00031473,0.00045788,0.00058087,0.00067063,0.00070989,
	0.00067875,0.00055792,0.00033311,-6.0069e-19,-0.00043109,-0.00093186,-0.0014549,
	-0.0019355,-0.002297,-0.0024587,-0.0023472,-0.0019085,
    -0.0011198,1.482e-18,0.0013842,
	0.0029171,0.0044391,0.0057588,0.0066712,0.00698,0.0065244
    ,0.0052038,0.0030013,-2.6087e-18,
	-0.0036091,-0.0075296,-0.011375,-0.014692,-0.017002,-0.017834
    ,-0.016781,-0.013538,-0.0079411,
	3.5333e-18,0.01009,0.021948,0.035025,0.048638,0.062016,0.074359
    ,0.0849,0.092964,0.098026,
	0.099752,0.098026,0.092964,0.0849,0.074359,0.062016,
    0.048638,0.035025,0.021948,0.01009,3.5333e-18,
	-0.0079411,-0.013538,-0.016781,-0.017834,-0.017002,
    -0.014692,-0.011375,-0.0075296,-0.0036091,
	-2.6087e-18,0.0030013,0.0052038,0.0065244,0.00698,0.0066712,
    0.0057588,0.0044391,0.0029171,
	0.0013842,1.482e-18,-0.0011198,-0.0019085,-0.0023472,-0.0024587,-0.002297,
    -0.0019355,-0.0014549,
	-0.00093186,-0.00043109,-6.0069e-19,0.00033311,0.00055792,0.00067875,0.00070989,0.00067063,
	0.00058087,0.00045788,0.00031473,0.00016019
};

uint8_t val;

void DrawBar(uint16_t bottomX, uint16_t bottomY, uint16_t maxHeight, uint16_t maxValue, float32_t value, uint16_t foreground, uint16_t background) 
	{
	uint16_t height,i;
	height = (uint16_t)((float32_t)value / (float32_t)maxValue * (float32_t)maxHeight);
	if (height == maxHeight) {
		TM_ILI9341_DrawLine(bottomX, bottomY, bottomX, bottomY - height, foreground);
	} else if (height < maxHeight) {
		TM_ILI9341_DrawLine(bottomX, bottomY, bottomX, bottomY - height, foreground);
		TM_ILI9341_DrawLine(bottomX, bottomY - height, bottomX, bottomY - maxHeight, background);
	}
	
		/* Print Little on LCD */
	  TM_ILI9341_Puts(100, 10, "FIR FILTER", &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_GREEN2);
	  TM_ILI9341_Puts(100, 230, "Frequncy (KHz)", &TM_Font_7x10, ILI9341_COLOR_BLACK, ILI9341_COLOR_WHITE); 
	 for (i = 0; i < 10 ; i++) {
	  TM_ILI9341_Puts((i*25.55+25), 200, "|", &TM_Font_7x10, ILI9341_COLOR_BLACK, ILI9341_COLOR_WHITE);
	 }
	 
	   TM_ILI9341_Puts((1*25.55+16), 210, "2.25", &TM_Font_7x10, ILI9341_COLOR_BLACK, ILI9341_COLOR_WHITE);
	  TM_ILI9341_Puts((3*25.55+16), 210, "6.75", &TM_Font_7x10, ILI9341_COLOR_BLACK, ILI9341_COLOR_WHITE);
	  TM_ILI9341_Puts((5*25.55+12), 210, "11.25", &TM_Font_7x10, ILI9341_COLOR_BLACK, ILI9341_COLOR_WHITE);
	  TM_ILI9341_Puts((7*25.55+10), 210, "15.75", &TM_Font_7x10, ILI9341_COLOR_BLACK, ILI9341_COLOR_WHITE);
	  TM_ILI9341_Puts((9*25.55+10), 210, "20.25", &TM_Font_7x10, ILI9341_COLOR_BLACK, ILI9341_COLOR_WHITE);
	 
	  TM_ILI9341_DrawLine(28,60,28,200,0x0005);
	  TM_ILI9341_DrawLine(28,200,280,200,0x0005);
	  
 }


/* convolution functon */
void convolutio(float32_t filter[N],float32_t input[SAMPLES],float32_t*conv_output)
{
      uint16_t n,m;
	   
	  for (n=0;n<SAMPLES;n++)
	   {
		   for(m=0;m<SAMPLES;m++)
			 {
				 if((n-m)>=0)
				 conv_output[n]= filter[m]*input[n-m];	
         		 
			 }
		 }
}

void frequncy_domain(void)
  {
		  TM_ILI9341_Fill(ILI9341_COLOR_WHITE);
			arm_cfft_radix4_instance_f32 S;	/* ARM CFFT module */
	    float32_t maxValue;				/* Max FFT value is stored here */
	    uint32_t maxIndex;				/* Index in Output array where max value is */
	    uint16_t i;
			for (i = 0; i < FFT_SIZE/2 ; i++) {
			/* Draw FFT results */
			DrawBar( 30+ 7 * i,
					200,
					FFT_BAR_MAX_HEIGHT,
					(uint16_t)maxValue,
					(float32_t)Output[(uint16_t)i],
					0x1234,
					0xFFFF
			);
		}	
	}
	
void time_domain(float32_t inputt[SAMPLES])
  {
		TM_ILI9341_Fill(ILI9341_COLOR_WHITE);
			uint16_t k;
	   float32_t size[SAMPLES]={0};
	   float32_t final[SAMPLES]={0};
     uint16_t biggest =0;
    			for (k=0; k < SAMPLES	; k= k+1)
	  				{
	  					if (inputt[k] > biggest)
	  					{
	  					biggest = inputt[k];
	  					}
	  				}

	  	for (k=0; k < SAMPLES; k= k+1)
	  				    {
	  				    	inputt[k]= (inputt[k]/biggest)*180;
									final[k]=180-inputt[k];
					        TM_ILI9341_DrawLine(28+2*k, 40, 28+2*k, 200, ILI9341_COLOR_BLACK);
									TM_ILI9341_DrawLine(28+(2*k+1), 40, 28+(2*k+1), 200, ILI9341_COLOR_BLACK);
									TM_ILI9341_DrawLine(28+2*k, round(final[k]), 29+2*k, round(final[k]), ILI9341_COLOR_WHITE);
									
									
									/* verctical axis line*/
									TM_ILI9341_DrawLine(28,60,28,200,0x0005);
									/* horizontal axis line*/
	                TM_ILI9341_DrawLine(28,200,280,200,0x0005);
								TM_ILI9341_Puts(100, 10, "TIME DOMAIN", &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_GREEN2);
	              TM_ILI9341_Puts(100, 230, "Time(s)", &TM_Font_7x10, ILI9341_COLOR_BLACK, ILI9341_COLOR_WHITE); 
	  				    }
  }


int main(void) {
	uint16_t i,display=0;
	float32_t fft_input[SAMPLES];

	/* Initialize system */
	SystemInit();
	
	/* Delay init */
	TM_DELAY_Init();
	
	/* Initialize LED's on board */
	TM_DISCO_LedInit();	
	/* Initialize LCD */
	TM_ILI9341_Init();
	TM_ILI9341_Rotate(TM_ILI9341_Orientation_Landscape_1);
	
	//TM_DAC_Init(TM_DAC2);
 /* Initialize ADC */
	TM_ADC_Init(ADC1, ADC_Channel_0);
	   //Push button initialize
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	GPIO_InitLEDDIE.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitLEDDIE.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitLEDDIE.GPIO_OType = GPIO_OType_PP;
	GPIO_InitLEDDIE.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitLEDDIE.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitLEDDIE);
	
		
	while (1) {
		for (i = 0; i < SAMPLES; i += 1) {
			/* Each 22us ~ 45kHz sample rate */
			Delay(21);		
			/* We assume that sampling and other stuff will take about 1us */		
			/* Real part, must be between -1 and 1 */
			Input[(uint16_t)i] = (float32_t)((float32_t)TM_ADC_Read(ADC1, ADC_Channel_0));
			/* Imaginary part */
			//Input[(uint16_t)(i + 1)] = 0;    
   // TM_DAC_SetValue(TM_DAC2,Input[i]);			
		}
		
	 /* computing convolution */
    
		convolutio(hn,Input,OutputLOW);	

		fft(OutputLOW);
		
			if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0) == Bit_SET){
			if(display == 0){
				display = 1;
				frequncy_domain();
			}
			else{
				display = 0;
				time_domain(OutputLOW);
			}
		}
		

	}
}