/*
 ES MUY POSIBLE QUE HAYA QUE ELEVAR LOS TAMAÑOS DE VARIABLES, AL USAR sin SIGNO QUEDA MAX 64K
 TODAS LAS ENTRADAS DEBEN ESTAR EN EL PORTB POR EL PULLUP
 EVALUAR CAMBIO DELAYS FUNCIONES POR TIEMPOS EN TMR2
 CORREGIR DESBORDAMIENTO EN TMR0 (TICKS) QUE NO SUPERE LOS 30D (PQ 30D? PQ ME GUSTO =D)

 LOGICA DE FUNCIONAMIENTO: RECIBO BOTELLA -> VOY A BOTELLA -> SUBO RAPIDO -> SUBO LENTO -> DOSIFICA
    -> VERIFICA DOSIFICACION -> BAJA RAPIDO -> INFORMO OK ->  ESPERO SIGUIENTE COMANDO
 */
#include <16F874A.h>
//#include "math.h" //Inecesaria de momento =D
#include "stdbool.h"
#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"

/*
//typedef signed int24 int24_t;
//typedef unsigned int24 uint24_t;

#define INT24_MAX  (8388607)
#define INT24_MIN  (-8388608)
#define UINT24_MAX (16777215)

//typedef signed int24 int_least24_t;
//typedef unsigned int24 uint_least24_t;

#define INT_LEAST24_MAX  (8388607)
#define INT_LEAST24_MIN  (-8388608)
#define UINT_LEAST24_MAX (16777215)

//typedef signed int24 int_fast24_t;
//typedef unsigned int24 uint_fast24_t;

#define INT_FAST24_MAX  (8388607)
#define INT_FAST24_MIN  (-8388608)
#define UINT_FAST24_MAX (16777215)

*/

#ifndef __CLION_IDE__
#fuses HS, NOWDT, NOPROTECT, NOLVP
#use delay(clock= 20000000)
#use rs232(baud=115200, xmit=pin_c6, rcv=pin_c7, bits=8)
#use standard_io(b)
#use standard_io(a)
#use standard_io(c)
#use standard_io(d)
#use standard_io(e)
#endif

//#USE TIMER(TIMER=1, TICK=1ms, BITS=32, NOISR) // da 816uS ahustar a 1mS

//DEFINICIONES COMODAS

#define horario 1
#define antihorario 0
#define lento 0
#define rapido 1

// DEFINICIONES DE PINES

// PUERTO A TODO ANALOGICAS Y CLOCK
//#define  PIN_A1
//#define  PIN_A0
//#define  PIN_A2
//#define  PIN_A3
//#define  PIN_A4
//#define  PIN_A5
// PIN A6 NO EXISTE
// PIN A7 NO EXISTE

// PUERTO B TODO ENTRADAS (PULLUP)
#define Derbot PIN_B0 // ENTRADA DE BOTON DE DERECHA
#define Ababot PIN_B1 // ENTRADA DE BOTON DE ABAJO
#define Arrbot PIN_B2// ENTRADA DE BOTON DE ARRIBA
#define Izqbot PIN_B3// ENTRADA DE BOTON IZQUIERDA
#define SwitchX PIN_B4 // LIMITE DE CARRERA X (POS0)
#define SwitchZ PIN_B5 // LIMITE DE CARRERA Z (POS0)
// B6 RESERVADO PARA COMUNICACION
// B7 RESERVADO PARA COMUNICACION

// PUERTO C
//#define  PIN_C0
/*#define algo PIN_C1
#define  PIN_C2
#define  PIN_C3
#define  PIN_C4
#define  PIN_C5
//#define TXUSART PIN_C6
//#define RXUSART PIN_C7
*/

// PUERTO D I/O GENERALES (SIN PULLUP)
#define sentido1 PIN_D0 // SENTIDO DE GIRO MOTOR 1
#define habil1 PIN_D1 // HABILITACION MOTOR 1
#define pulsos1 PIN_D2 // PULSOS MOTOR 1

#define sentido2 PIN_D3 //SENTIDO DE GIRO MOTOR 2
#define habil2 PIN_D4 // HABILITACION MOTOR 2
#define pulsos2 PIN_D5 // PULSOS MOTOR 2

#define CALbot PIN_D6 // BOTON DE CALIBRACION
#define Refrigeracion PIN_D7

// PUERTO E
//#define PIN_E0
//#define PIN_E1
//#define PIN_E2


// DEFINICIONES DE TIEMPOS

#define RTon 10 // tiempo de pulso en microsegundos RAPIDO
#define RToff 10 // tiempo de pulso en microsegundos RAPIDO

#define LTon 10 // tiempo de pulso en microsegundos LENTO
#define LToff 500 // tiempo de pulso en microsegundos LENTO


// DEFINICIONES DE PASOS

#define PasosCalibX 1000 // PASOS QUE MUEVE LENTO PARA CALIBRAR X
#define PasosCalibZ 1000 // PASOS QUE MUEVE LENTO PARA CALIBRAR Z
#define Pasosdoficacion 100 // PASOS QUE MIDE LA DOSIFICACION
#define constDiagonal 0.70710678118654 // es un monton de resolucion ya

// POSICIONES BOTELLAS

#define Botella1X 100
#define Botella1Z 100
#define Botella2X 200
#define Botella2Z 100
#define Botella3X 300
#define Botella3Z 100
#define Botella4X 400
#define Botella4Z 100
#define Botella5X 500
#define Botella5Z 100
#define Botella6X 600
#define Botella6Z 100
#define Botella7X 700
#define Botella7Z 100
#define Botella8X 800
#define Botella8Z 100
#define Botella9X 900
#define Botella9Z 100
#define mezclaX 1000
#define mezclaZ 10


// OTRAS DEFINICIONES

#define HisteresisTemp 1.01 // HISTERESIS DE TEMPERATURA -> 1%


// VARIABLES DE CONTROL

uint16_t pasosX = 0; // pasos X absolutos desde el 0
uint16_t pasosZ = 0; // pasos Z absolutos desde el 0
uint16_t pasosmovX = 0; // pasos a mover EN EJE X
uint16_t pasosmovZ = 0; // PASOS A MOVER EN EJE Z
uint16_t botellaX = 0; // POS X DE BOTELLA SELECCIONADA
uint16_t botellaZ = 0; // POS Z DE BOTELLA SELECCIONADA


// OTRAS VARIABLES
uint8_t botella = 0; // NUMERO DE BOTELLA / MEZCLA DESDE USB
uint32_t TiempoTemp = 100; // TIEMPO verificacion enfriamiento en S *100
//bool TiempoAnalisis = false; // VARIABLE DE CONTROL DE TIEMPO DE ANALISIS
float TempActual = 0; // TEMPERATURA DE LA MUESTRA
float TempDeseada = 0;
uint32_t TiempoActual = 0; // TIEMPO DE ANALISIS
uint32_t TiempoPrevio = 0; // TIEMPO DE ANALISIS
uint32_t TICKS = 0; // TICKS 1mS

// VARIABLES DE COMUNICACION

char valorRecepcion = 0;
int leerMensaje = false;
int posicionMensaje = 0;
char codigoAccion;
char valorMensaje[3];

#ifndef __CLION_IDE__
#INT_RDA
#endif

// FUNCIONES

void Punto_0(); // VOID CALIBRACION PUNTO 0
void ERROR(); // COMUNICA ERROR
void Mover(); // LLAMA A LAS FUNCIONES DE MOVIMIENTO INTEGRAS
void MovIzq(uint16_t pasos, bool velocidad); // MUEVE SOLO IZQ
void MovDer(uint16_t pasos, bool velocidad); // MUEVE SOLO DER
void MovArr(uint16_t pasos, bool velocidad); // MUEVE SOLO ARB
void MovAba(uint16_t pasos, bool velocidad); // MUEVE SOLO ABA
void Mov45(uint16_t pasos, bool velocidad); //+Z +X
void Mov135(uint16_t pasos, bool velocidad);// +Z -X
void Mov225(uint16_t pasos, bool velocidad);// -Z -X
void Mov315(uint16_t pasos, bool velocidad);// -Z +X
void ControlTemp(); // CONTROLA LA TEMPERATURA
void ObtenerTemp(); // OBTIENE LA TEMPERATURA del sensor (QUE SENSOR)???????

void Punto_0() { // FUNCION QUE VA AL PUNTO 0
    if (input(SwitchX) == 1) {
        while (input(SwitchX) == 1) {
            MovIzq(1, rapido);
        }
    }
    if (input(SwitchZ) == 1) {
        while (input(SwitchZ) == 1) {
            MovAba(1, rapido);
        }
    }
    MovDer(PasosCalibX, rapido);
    MovArr(PasosCalibZ, rapido);
    while (input(SwitchX) == 1) {
        MovIzq(1, lento);
    }
    while (input(SwitchZ) == 1) {
        MovAba(1, lento);
    }
    pasosX = 0;
    pasosZ = 0;
}

void MovDer(uint16_t pasos, bool velocidad) {
    output_bit(sentido1, antihorario);  //sentido antihorario
    output_bit(sentido2, antihorario);  //sentido antihorario

    if (velocidad == rapido) {
        for (uint16_t i = 0; i < pasos; ++i) {
            output_high(pulsos1);
            output_high(pulsos2);
            delay_us(RTon);
            output_low(pulsos1);
            output_low(pulsos2);
            delay_us(RToff);

        }
    }
    else {
        for (uint16_t i = 0; i < pasos; ++i) {
            output_high(pulsos1);
            output_high(pulsos2);
            delay_us(LTon);
            output_low(pulsos1);
            output_low(pulsos2);
            delay_us(LToff);
        }
    }
    pasosX = pasosX + pasos;
}

void MovIzq(uint16_t pasos, bool velocidad) {
    output_bit(sentido1, horario);  //sentido horario
    output_bit(sentido2, horario);  //sentido horario

    if (velocidad == rapido) {
        for (uint16_t i = 0; i < pasos; ++i) {
            output_high(pulsos1);
            output_high(pulsos2);
            delay_us(RTon);
            output_low(pulsos1);
            output_low(pulsos2);
            delay_us(RToff);

        }
    }
    else {
        for (uint16_t i = 0; i < pasos; ++i) {
            output_high(pulsos1);
            output_high(pulsos2);
            delay_us(LTon);
            output_low(pulsos1);
            output_low(pulsos2);
            delay_us(LToff);
        }
    }
    pasosX = pasosX - pasos;
}

void MovArr(uint16_t pasos, bool velocidad) {
    output_bit(sentido1, horario);  //sentido horario
    output_bit(sentido2, antihorario);  //sentido horario

    if (velocidad == rapido) {
        for (uint16_t i = 0; i < pasos; ++i) {
            output_high(pulsos1);
            output_high(pulsos2);
            delay_us(RTon);
            output_low(pulsos1);
            output_low(pulsos2);
            delay_us(RToff);

        }
    }
    else {
        for (uint16_t i = 0; i < pasos; ++i) {
            output_high(pulsos1);
            output_high(pulsos2);
            delay_us(LTon);
            output_low(pulsos1);
            output_low(pulsos2);
            delay_us(LToff);
        }
    }
    pasosZ = pasosZ + pasos;
}

void MovAba(uint16_t pasos, bool velocidad) {
    output_bit(sentido1, antihorario);  //sentido horario
    output_bit(sentido2, horario);  //sentido horario

    if (velocidad == rapido) {
        for (uint16_t i = 0; i < pasos; ++i) {
            output_high(pulsos1);
            output_high(pulsos2);
            delay_us(RTon);
            output_low(pulsos1);
            output_low(pulsos2);
            delay_us(RToff);

        }
    }
    else {
        for (uint16_t i = 0; i < pasos; ++i) {
            output_high(pulsos1);
            output_high(pulsos2);
            delay_us(LTon);
            output_low(pulsos1);
            output_low(pulsos2);
            delay_us(LToff);
        }
    }
    pasosZ = pasosZ - pasos;
}

void Mov45(uint16_t pasos, bool velocidad) {//+Z + X -> - A
    //output_bit(sentido1, antihorario);  //sentido horario
    output_bit(sentido2, antihorario);  //sentido horario

    if (velocidad == rapido) {
        for (uint16_t i = 0; i < pasos; ++i) {
            //output_high(pulsos1); -> NO TIENE QUE MOVERLO
            output_high(pulsos2);
            delay_us(RTon);
            //output_low(pulsos1); -> NO TIENE QUE MOVERLO
            output_low(pulsos2);
            delay_us(RToff);

        }
    }
    else {
        for (uint16_t i = 0; i < pasos; ++i) {
            //output_high(pulsos1);-> NO TIENE QUE MOVERLO
            output_high(pulsos2);
            delay_us(LTon);
            //output_low(pulsos1); -> NO TIENE QUE MOVERLO
            output_low(pulsos2);
            delay_us(LToff);
        }
    }

    pasosZ = pasosZ + (pasos * constDiagonal);
    pasosX = pasosX + (pasos * constDiagonal);

}

void Mov135(uint16_t pasos, bool velocidad) { // +Z -X -> H -
    output_bit(sentido1, horario);  //sentido horario
    //output_bit(sentido2, horario);  //sentido horario

    if (velocidad == rapido) {
        for (uint16_t i = 0; i < pasos; ++i) {
            output_high(pulsos1);
            //output_high(pulsos2); -> NO TIENE QUE MOVERLO
            delay_us(RTon);
            output_low(pulsos1);
            //output_low(pulsos2); -> NO TIENE QUE MOVERLO
            delay_us(RToff);

        }
    }
    else {
        for (uint16_t i = 0; i < pasos; ++i) {
            output_high(pulsos1);
            //output_high(pulsos2);
            delay_us(LTon);
            output_low(pulsos1);
            //output_low(pulsos2);
            delay_us(LToff);
        }
    }

    pasosZ = pasosZ + (pasos * constDiagonal);
    pasosX = pasosX - (pasos * constDiagonal);

}

void Mov225(uint16_t pasos, bool velocidad) {// -Z -X -> - H

    //output_bit(sentido1, antihorario);  //sentido horario
    output_bit(sentido2, horario);  //sentido horario

    if (velocidad == rapido) {
        for (uint16_t i = 0; i < pasos; ++i) {
            //output_high(pulsos1); -> NO TIENE QUE MOVERLO
            output_high(pulsos2);
            delay_us(RTon);
            //output_low(pulsos1); -> NO TIENE QUE MOVERLO
            output_low(pulsos2);
            delay_us(RToff);

        }
    }
    else {
        for (uint16_t i = 0; i < pasos; ++i) {
            //output_high(pulsos1); -> NO TIENE QUE MOVERLO
            output_high(pulsos2);
            delay_us(LTon);
            //output_low(pulsos1); -> NO TIENE QUE MOVERLO
            output_low(pulsos2);
            delay_us(LToff);
        }
    }

    pasosZ = pasosZ - (pasos * constDiagonal);
    pasosX = pasosX - (pasos * constDiagonal);


}

void Mov315(uint16_t pasos, bool velocidad) {// -Z +X
    output_bit(sentido1, antihorario);  //sentido horario
    //output_bit(sentido2, horario);  //sentido horario

    if (velocidad == rapido) {
        for (uint16_t i = 0; i < pasos; ++i) {
            output_high(pulsos1);
            //output_high(pulsos2); -> NO TIENE QUE MOVERLO
            delay_us(RTon);
            output_low(pulsos1);
            //output_low(pulsos2); -> NO TIENE QUE MOVERLO
            delay_us(RToff);

        }
    }
    else {
        for (uint16_t i = 0; i < pasos; ++i) {
            output_high(pulsos1);
            //output_high(pulsos2);  -> NO TIENE QUE MOVERLO
            delay_us(LTon);
            output_low(pulsos1);
            //output_low(pulsos2);  -> NO TIENE QUE MOVERLO
            delay_us(LToff);
        }
    }

    pasosZ = pasosZ - (pasos * constDiagonal);
    pasosX = pasosX + (pasos * constDiagonal);

}

void Mover() {
    switch (botella) {
        case 1:
            pasosmovX = pasosX - Botella1X;
            pasosmovZ = pasosZ - Botella1Z;
            botellaX = Botella1X;
            botellaZ = Botella1Z;
            break;
        case 2:
            pasosmovX = pasosX - Botella2X;
            pasosmovZ = pasosZ - Botella2Z;
            botellaX = Botella2X;
            botellaZ = Botella2Z;
            break;
        case 3:
            pasosmovX = pasosX - Botella3X;
            pasosmovZ = pasosZ - Botella3Z;
            botellaX = Botella3X;
            botellaZ = Botella3Z;
            break;
        case 4:
            pasosmovX = pasosX - Botella4X;
            pasosmovZ = pasosZ - Botella4Z;
            botellaX = Botella4X;
            botellaZ = Botella4Z;
            break;
        case 5:
            pasosmovX = pasosX - Botella5X;
            pasosmovZ = pasosZ - Botella5Z;
            botellaX = Botella5X;
            botellaZ = Botella5Z;
            break;
        case 6:
            pasosmovX = pasosX - Botella6X;
            pasosmovZ = pasosZ - Botella6Z;
            botellaX = Botella6X;
            botellaZ = Botella6Z;
            break;
        case 7:
            pasosmovX = pasosX - Botella7X;
            pasosmovZ = pasosZ - Botella7Z;
            botellaX = Botella7X;
            botellaZ = Botella7Z;
            break;
        case 8:
            pasosmovX = pasosX - Botella8X;
            pasosmovZ = pasosZ - Botella8Z;
            botellaX = Botella8X;
            botellaZ = Botella8Z;
            break;
        case 9:
            pasosmovX = pasosX - Botella9X;
            pasosmovZ = pasosZ - Botella9Z;
            botellaX = Botella9X;
            botellaZ = Botella9Z;
            break;
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            pasosmovX = pasosX - mezclaX;
            pasosmovZ = pasosZ - mezclaZ;
            botellaX = mezclaX;
            botellaZ = mezclaZ;
            break;
        default:
            ERROR();
            break;
    }
    if (pasosX == botellaX && pasosZ == botellaZ) {
        //no se mueve
        //posible error
    }
    if (pasosX == botellaX && pasosZ != botellaZ) {
        if (pasosZ > botellaZ) {
            //mover -Z
            MovAba(pasosmovZ, rapido);
            //pasosZ = botellaZ;
        }
        else {
            //mover +Z
            MovArr(pasosmovZ, rapido);
            //pasosZ = botellaZ;
        }
    }
    if (pasosX = !botellaX && pasosZ == botellaZ) {
        if (pasosX > botellaX) {
            //mover -X
            MovIzq(pasosmovX, rapido);
            //pasosX = botellaX;
        }
        else {
            //mover +X
            MovDer(pasosmovX, rapido);
            //pasosX = botellaX;
        }
    }
    if (pasosX > botellaX && pasosZ > botellaZ) {
        //mover -X -Z
        if (pasosmovX > pasosmovZ) {
            Mov225(pasosmovZ, rapido);//creo que es * sino probar /
            pasosmovX = pasosmovX - pasosmovZ;
            MovIzq(pasosmovX, rapido);
        }
        else {
            Mov225(pasosmovX, rapido);//creo que es * sino probar /
            pasosmovZ = pasosmovZ - pasosmovX;
            MovAba(pasosmovZ, rapido);
        }

        //pasosX = pasosX * constDiagonal; //creo que es * sino probar /
        //Mov225(pasosX, rapido);

    }
    if (pasosX > botellaX && pasosZ < botellaZ) {
        //mover -X +Z
        if (pasosmovX > pasosmovZ) {
            Mov135(pasosmovZ, rapido);//creo que es * sino probar /
            pasosmovX = pasosmovX - pasosmovZ;
            MovIzq(pasosmovX, rapido);
        }
        else {
            Mov135(pasosmovX, rapido);//creo que es * sino probar /
            pasosmovZ = pasosmovZ - pasosmovX;
            MovArr(pasosmovZ, rapido);
        }
    }
    if (pasosX < botellaX && pasosZ > botellaZ) {
        // mover +X -Z
        if (pasosmovX > pasosmovZ) {
            Mov315(pasosmovZ, rapido);//creo que es * sino probar /
            pasosmovX = pasosmovX - pasosmovZ;
            MovDer(pasosmovX, rapido);
        }
        else {
            Mov315(pasosmovX, rapido);//creo que es * sino probar /
            pasosmovZ = pasosmovZ - pasosmovX;
            MovAba(pasosmovZ, rapido);
        }
    }
    if (pasosX < botellaX && pasosZ < botellaZ) {
        //mover +X +Z
        if (pasosmovX > pasosmovZ) {
            Mov45(pasosmovZ, rapido);//creo que es * sino probar /
            pasosmovX = pasosmovX - pasosmovZ;
            MovDer(pasosmovX, rapido);
        }
        else {
            Mov45(pasosmovX, rapido);//creo que es * sino probar /
            pasosmovZ = pasosmovZ - pasosmovX;
            MovArr(pasosmovZ, rapido);
        }

    }

    MovArr(Pasosdoficacion, lento);
    MovAba(Pasosdoficacion, rapido);

    botella = 0; //LIMPIEZA
    botellaZ = 0; //LIMPIEZA
    botellaX = 0; //LIMPIEZA
    pasosmovZ = 0; //LIMPIEZA
    pasosmovX = 0; //LIMPIEZA
}

void ControlTemp() {

    if ((TempActual * HisteresisTemp) > TempDeseada) {
        output_high(Refrigeracion);
    }
    else {
        output_low(Refrigeracion);
    }

}

void ObtenerTemp() {}


#int_timer1

void timer1_isr() {
    TICKS++; // Incrementa la variable TICKS cada vez que ocurre una interrupción del Timer1
    set_timer1(3036); // Carga el valor al Timer1 para que se desborde en 1ms
    clear_interrupt(INT_TIMER1); // Limpia la bandera de interrupción del Timer1
}

void RDA_isr() {
    valorRecepcion = getc();
    if (valorRecepcion == '*') {
        if (leerMensaje == false) {
            leerMensaje = true;
        }
        else {
            leerMensaje = false;
            uint16_t pasos = atoi(valorMensaje);
            switch (codigoAccion) {
                case 'A':
                    for (unsigned int i = 0; i < pasos; ++i) {
                        output_bit(sentido1, antihorario);  //sentido antihorario
                        output_bit(sentido2, antihorario);  //sentido antihorario

                        output_high(pulsos1);
                        output_high(pulsos2);
                        delay_us(RTon);
                        output_low(pulsos1);
                        output_low(pulsos2);
                        delay_us(RToff);
                    }
                    break;
                default:
                    break;
            }
        }
    }
    if (leerMensaje == 1) {
        if (posicionMensaje == 0) {
            posicionMensaje++;
            codigoAccion = valorRecepcion;
        }
        else {
            valorMensaje[posicionMensaje++ - 1] = valorRecepcion;

        }
    }
}


int main() {
    enable_interrupts(INT_RDA); // Habilita la interrupción del puerto serial
    setup_timer_1(T1_INTERNAL | T1_DIV_BY_8); // Configura Timer1 con oscilador interno y prescaler 1:8
    enable_interrupts(INT_TIMER1); // Habilita la interrupción del Timer1
    set_timer1(3036); // Carga el valor al Timer1 para que se desborde en 1ms
    enable_interrupts(GLOBAL); // Habilita las interrupciones globales
    //input_pullup(SwitchX);
    //input_pullup(SwitchZ);
    //PORT_b_PULLUPS()
    port_b_pullups(true); // FUNCIONA NO TOCAR


//output_high(habil);  //deshabilito el driver
//output_low(pulsos);  //salida de pulsos a cero

    input(Derbot);
    input(Izqbot);
    input(Arrbot);
    input(Ababot);
    input(CALbot);

    output_low(Refrigeracion);

/*while(1){
if(input(PIN_B3)==0) //si quiero mover el motor en un sentido (horario)
	{
    delay_ms(10);
		while(input(PIN_B3)==0)
		{
            output_low(habil);  //habilito el driver
            output_low(sentido);  //sentido horario
            output_high(pulsos);
            delay_us(10);
            output_low(pulsos);
            delay_us(velocidad);
		}
     output_high(habil);  //deshabilito el driver
	}
if(input(PIN_B4)==0) //si quiero mover el motor en un sentido (horario)
	{
    delay_ms(10);
		while(input(PIN_B4)==0)
		{
            output_low(habil);  //habilito el driver
            output_high(sentido);  //sentido antihorario
            output_high(pulsos);
            delay_us(10);
            output_low(pulsos);
            delay_us(velocidad);
		}
     output_high(habil);  //deshabilito el driver
	}

if(input(PIN_B5)==0)
	{
    delay_ms(10);
		if(input(PIN_B5)==0)
		{
	while(!input(PIN_B5));
		output_high(habil);  //deshabilito el driver
		}
	}
}
*/
    //output_low(habil1);  //Habilito el driver
    output_low(habil1);  //habilito el driver1
    output_low(habil2);  //Habilito el driver
    //output_high(habil1);  //habilito el driver1 NO
    //output_high(habil2);  //habilito el driver2 NO

    /* for (uint16_t i = 0; i < 20000; ++i) { // FUNCION SUBIDA???????
         output_bit(sentido1, antihorario);  //sentido antihorario
         output_bit(sentido2, antihorario);  //sentido antihorario

         output_high(pulsos1);
         output_high(pulsos2);
         delay_us(RTon);
         output_low(pulsos1);
         output_low(pulsos2);
         delay_us(RToff);
     }
 */ // FUNCION SUBIDA???????
    TiempoPrevio = TICKS;
    TiempoActual = TICKS;

    while (1) {
        if (input(Derbot) == 1) {//si quiero mover el motor en un sentido (horario)
            delay_ms(10);
            while (input(Derbot) == 1) {

                output_bit(sentido1, antihorario);  //sentido antihorario
                output_bit(sentido2, antihorario);  //sentido antihorario

                output_high(pulsos1);
                output_high(pulsos2);
                delay_us(RTon);
                output_low(pulsos1);
                output_low(pulsos2);
                delay_us(RToff);
            }

        }

        if (input(Izqbot) == 1) {//si quiero mover el motor en un sentido (horario)
            delay_ms(10);
            while (input(Izqbot) == 1) {
                output_bit(sentido1, horario);  //sentido horario
                output_bit(sentido2, horario);  //sentido horario

                output_high(pulsos1);
                output_high(pulsos2);
                delay_us(RTon);
                output_low(pulsos1);
                output_low(pulsos2);
                delay_us(RToff);
            }
        }

        if (input(Arrbot) == 1) {//si quiero mover el motor en un sentido (horario)


            delay_ms(10);
            while (input(Arrbot) == 1) {

                output_bit(sentido1, horario);  //sentido horario
                output_bit(sentido2, antihorario);  //sentido horario

                output_high(pulsos1);
                output_high(pulsos2);
                delay_us(RTon);
                output_low(pulsos1);
                output_low(pulsos2);
                delay_us(RToff);
            }
        }

        if (input(Ababot) == 1) { //si quiero mover el motor en un sentido (horario)


            delay_ms(10);
            while (input(Ababot) == 1) {

                output_bit(sentido1, antihorario);  //sentido horario
                output_bit(sentido2, horario);  //sentido horario

                output_high(pulsos1);
                output_high(pulsos2);
                delay_us(RTon);
                output_low(pulsos1);
                output_low(pulsos2);
                delay_us(RToff);
            }
        }

        if (input(CALbot) == 1) { //si quiero mover el motor en un sentido (horario)
            delay_ms(10);
            if (input(CALbot) == 1) {
                Punto_0();
            }
        }

        TiempoActual = TICKS;
        if ((TiempoActual - TiempoPrevio) > TiempoTemp) {
            ObtenerTemp();
            ControlTemp();
            TiempoPrevio = TICKS;
        }

    }
    return 0;
}
