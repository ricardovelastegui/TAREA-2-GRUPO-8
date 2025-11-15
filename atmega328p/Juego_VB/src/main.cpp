#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>

// =========================================================
// ===  CONFIGURACIÓN DE MATRIZ DE LEDs 8x8 CON ATmega328P ===
// =========================================================

// Barrido de filas (PORTD)
unsigned char FILAS[8] = {1, 2, 4, 8, 16, 32, 64, 128};

// Mapa desplazable (nivel del juego)
unsigned char MAPA[120] = {
    // primeras columnas vacías para no morir al inicio
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x80, 0xC0, 0xC0, 0xE0, 0xE0, 0x80, 0x80,
    0x80, 0x80, 0xC0, 0xC0, 0x80, 0x80, 0xE0, 0xE0,
    0x80, 0x80, 0x80, 0xC0, 0xC0, 0x80, 0x86, 0x86,
    0x00, 0x00, 0x80, 0x80, 0xC0, 0xC0, 0x80, 0x80,
    0x00, 0x00, 0x80, 0x80, 0x80, 0xE0, 0xE0, 0x80,
    0x80, 0x80, 0xC0, 0xC0, 0xE0, 0xE0, 0x80, 0x80,
    0x80, 0x80, 0xC0, 0xC0, 0x80, 0x80, 0xE0, 0xE0,
    0x80, 0x80, 0x80, 0xC0, 0xC0, 0x80, 0x86, 0x86,
    0x00, 0x00, 0x80, 0x80, 0xC0, 0xC0, 0x80, 0x80,
    0x00, 0x00, 0x80, 0x80, 0x80, 0xE0, 0xE0, 0x80};

// Caras finales
unsigned char CARA_GANAR[8] = {0x00, 0x2C, 0x4C, 0x40, 0x40, 0x4C, 0x2C, 0x00};
unsigned char CARA_PERDER[8] = {0x00, 0x4C, 0x2C, 0x20, 0x20, 0x2C, 0x4C, 0x00};

// Sprites del jugador
unsigned char JUGADOR_SUELO[8] = {0x00, 0x10, 0x78, 0x10, 0x00, 0x00, 0x00, 0x00};
unsigned char JUGADOR_SALTO[8] = {0x00, 0x10, 0x28, 0x1C, 0x28, 0x04, 0x00, 0x00};
unsigned char JUGADOR_SALTO2[8] = {0x00, 0x08, 0x14, 0x0E, 0x14, 0x02, 0x00, 0x00};

// =========================================================
// ===  VARIABLES DEL JUEGO  ===
// =========================================================

int jugador_y = 4;  // posición vertical (fila superior del personaje)
int salto = 0;      // 0=sin salto, 1=salto simple, 2=salto doble
int altura = 0;     // frames de subida/bajada
int mapa_index = 0; // desplazamiento del mapa
char vivo = 1;      // bandera de estado

// =========================================================
// ===  FUNCIONES AUXILIARES  ===
// =========================================================

// Limpia toda la matriz
void limpiar_matriz(void)
{
  PORTB = 0x00;
  PORTD = 0x00;
}

// Detecta botones
// PC0 = salto simple, PC1 = salto doble
char leer_boton(void)
{
  if (!(PINC & (1 << PC0)))
    return 1; // salto
  if (!(PINC & (1 << PC1)))
    return 2; // salto doble
  return 0;
}

// Dibuja un frame estable (sin parpadeo)
void dibujar_frame(unsigned char *sprite, int mapa_index)
{
  for (int repet = 0; repet < 15; repet++)
  { // se repite rápido para evitar parpadeo
    for (int fila = 0; fila < 8; fila++)
    {
      PORTD = FILAS[fila];
      unsigned char mapa = MAPA[mapa_index + fila];
      unsigned char jugador = sprite[fila];
      PORTB = ~(mapa | jugador);
      _delay_us(600); // breve retardo por fila
    }
  }
}

// Dibuja una carita (ganar/perder)
void mostrar_cara(unsigned char *cara)
{
  for (int t = 0; t < 600; t++)
  {
    for (int f = 0; f < 8; f++)
    {
      PORTD = FILAS[f];
      PORTB = ~cara[f];
      _delay_us(700);
    }
  }
}

// =========================================================
// ===  PROGRAMA PRINCIPAL  ===
// =========================================================

int main(void)
{
  DDRB = 0xFF;   // columnas
  DDRD = 0xFF;   // filas
  DDRC &= 0xF0;  // PC0 y PC1 entradas
  PORTC |= 0x0F; // pull-ups

  limpiar_matriz();

  while (vivo)
  {
    // ----- LECTURA DE BOTONES -----
    char btn = leer_boton();
    if (btn == 1 && salto == 0)
    {
      salto = 1;
      altura = 0;
    }
    if (btn == 2 && salto == 0)
    {
      salto = 2;
      altura = 0;
    }

    // ----- FÍSICA DEL SALTO -----
    if (salto == 1)
    { // salto simple
      if (altura < 3)
        jugador_y--;
      else if (altura < 6)
        jugador_y++;
      else
        salto = 0;
      altura++;
    }
    else if (salto == 2)
    { // salto doble
      if (altura < 5)
        jugador_y--;
      else if (altura < 10)
        jugador_y++;
      else
        salto = 0;
      altura++;
    }

    if (jugador_y < 0)
      jugador_y = 0;
    if (jugador_y > 4)
      jugador_y = 4; // piso

    // ----- SELECCIÓN DE SPRITE -----
    unsigned char *sprite;
    if (salto == 1)
      sprite = JUGADOR_SALTO;
    else if (salto == 2)
      sprite = JUGADOR_SALTO2;
    else
      sprite = JUGADOR_SUELO;

    // ----- DIBUJA ESCENA -----
    dibujar_frame(sprite, mapa_index);

    // ----- DETECCIÓN DE COLISIÓN -----
    for (int f = 0; f < 8; f++)
    {
      if (MAPA[mapa_index + f] & sprite[f])
      {
        vivo = 0; // colisión = pierde
      }
    }

    // ----- AVANZA EL MAPA -----
    mapa_index++;
    if (mapa_index > sizeof(MAPA) - 8)
    {
      mostrar_cara(CARA_GANAR);
      break;
    }

    _delay_ms(60); // velocidad general del juego
  }

  // Si perdió
  if (!vivo)
  {
    mostrar_cara(CARA_PERDER);
  }

  limpiar_matriz();
  return 0;
}