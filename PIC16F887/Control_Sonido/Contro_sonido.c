//-----------------------------------------------------------
// CONTROLADOR PIC16F887 PARA EL JUEGO 8x8 CON ATMEGA328P
//-----------------------------------------------------------

// ----- ENTRADAS DESDE BOTONES -----
sbit BTN_SALTO at RB0_bit;
sbit BTN_DOBLE at RB1_bit;
sbit BTN_BAJAR at RB2_bit;

// ----- SALIDAS HACIA ATMEGA -----
sbit OUT_SALTO at RC0_bit;
sbit OUT_DOBLE at RC1_bit;
sbit OUT_BAJAR at RC2_bit;

// ----- ENTRADAS DESDE ATMEGA -----
sbit WIN_FLAG at RD1_bit;
sbit LOSE_FLAG at RD2_bit;
sbit START_FLAG at RD3_bit;

// ----- SALIDA AUDIO (RD0) -----
#define SOUND_PORT PORTD
#define SOUND_PIN 0


// ===========================================================
// ============= VARIABLES DE CONTROL DEL REPRODUCTOR ========
// ===========================================================

// Temporizador simple tipo “tick” (simula millis)
unsigned int tick = 0;

// Índice de la nota en reproducción
unsigned short indice_nota = 0;

// Flags del sistema (DECLARACIÓN - SIN ASIGNACIÓN INICIAL)
bit melodia_activa;
bit melodia_ganar;
bit melodia_perder;
bit juego_activo;

// Bandera para evitar reinicio constante de fondo
bit fondo_iniciado;


// ===========================================================
// ================== MELODÍAS (en ROM = const) ===============
// ===========================================================

// Melodía de fondo
const unsigned int fondo_notas[] = {262, 330, 392, 330};
const unsigned int fondo_tiempos[] = {120, 120, 120, 120};
unsigned short fondo_total = 4;

// Melodía de ganar
const unsigned int ganar_notas[] = {392, 440, 494};
const unsigned int ganar_tiempos[] = {150, 200, 250};
unsigned short ganar_total = 3;

// Melodía de perder
const unsigned int perder_notas[] = {392, 330, 262};
const unsigned int perder_tiempos[] = {200, 200, 300};
unsigned short perder_total = 3;


// ===========================================================
// =========== REPRODUCTOR GENERAL NO BLOQUEANTE =============
// ===========================================================

void melodias_update() {

    // Incrementa el temporizador
    tick++;
    if (tick < 80) return; // Ajusta velocidad musical
    tick = 0;

    // Si no hay melodía activa, salir
    if (!melodia_activa) return;

    // -------------------- MELODÍA DE GANAR / PERDER --------------------
    if (melodia_ganar || melodia_perder) {

        const unsigned int *notas = melodia_ganar ? ganar_notas : perder_notas;
        const unsigned int *tiempos = melodia_ganar ? ganar_tiempos : perder_tiempos;
        unsigned short total = melodia_ganar ? ganar_total : perder_total;

        // Sound_Play es BLOQUEANTE. Esto detiene el main loop por el tiempo de la nota.
        Sound_Play(notas[indice_nota], tiempos[indice_nota]);

        indice_nota++;

        if (indice_nota >= total) {
            melodia_ganar = 0;
            melodia_perder = 0;
            melodia_activa = 0; // Detiene el sonido al final
            indice_nota = 0;
        }
        return;
    }

    // -------------------- MELODÍA DE FONDO --------------------
    // Sound_Play es BLOQUEANTE.
    Sound_Play(fondo_notas[indice_nota], fondo_tiempos[indice_nota]);
    indice_nota++;

    if (indice_nota >= fondo_total)
        indice_nota = 0;
}


// ===========================================================
// ======================== INICIADORES =======================
// ===========================================================

void iniciar_melodia_fondo() {
    melodia_activa = 1;
    melodia_ganar = 0;
    melodia_perder = 0;
    indice_nota = 0;
    fondo_iniciado = 1; // Bandera añadida
}

void iniciar_ganar() {
    melodia_activa = 1;
    melodia_ganar = 1;
    melodia_perder = 0;
    indice_nota = 0;
    fondo_iniciado = 0;
}

void iniciar_perder() {
    melodia_activa = 1;
    melodia_perder = 1;
    melodia_ganar = 0;
    indice_nota = 0;
    fondo_iniciado = 0;
}


// ===========================================================
// ===================== CONFIGURACIÓN PIC ====================
// ===========================================================

void setup() {

    // ENTRADAS (botones)
    TRISB = 0xFF;

    // ACTIVAR PULL-UPS INTERNOS EN PORTB
    OPTION_REG.F7 = 0;       // Habilita pull-ups internos globales
    WPUB = 0b00000111;       // Pull-ups SOLO en RB0, RB1, RB2

    // SALIDAS hacia ATMEGA
    TRISC = 0x00;

    // RD0 = sonido (salida)
    // RD1, RD2, RD3 = entradas desde ATMEGA
    TRISD = 0b00001110;

    PORTC = 0;
    PORTD = 0;

    Sound_Init(&SOUND_PORT, SOUND_PIN);
}




// ===========================================================
// ======================== MAIN LOOP =========================
// ===========================================================

void main() {
    // 1. Inicialización de banderas de bit (CORRECCIÓN CRÍTICA)
    melodia_activa = 0;
    melodia_ganar = 0;
    melodia_perder = 0;
    juego_activo = 0;
    fondo_iniciado = 0;

    setup();

    while(1) {

        // ===================================================
        // START = 1 (ALTO) ? juego detenido y sonido apagado
        // ===================================================
        if (START_FLAG == 1 && juego_activo == 1) {
            juego_activo = 0;
            melodia_activa = 0; // Detiene la música
            fondo_iniciado = 0; // Permite reiniciar fondo
        }

        // ===================================================
        // START = 0 (BAJO) ? juego activo, iniciar melodía de fondo
        // ===================================================
        // Se añade la condición fondo_iniciado para que solo se llame una vez
        if (START_FLAG == 0 && juego_activo == 0) {
            juego_activo = 1;
            iniciar_melodia_fondo();
        }

        // ===================================================
        // BOTONES ? ATMEGA
        // ===================================================
        OUT_SALTO = BTN_SALTO;   // 1 = suelto, 0 = presionado
        OUT_DOBLE = BTN_DOBLE;
        OUT_BAJAR = BTN_BAJAR;



        // ===================================================
        // VICTORIA / DERROTA DESDE ATMEGA (solo si el juego está activo)
        // ===================================================
        if (juego_activo == 1) {
            if (WIN_FLAG == 1) iniciar_ganar();
            if (LOSE_FLAG == 1) iniciar_perder();
        }


        // ===================================================
        // ACTUALIZAR REPRODUCTOR PERIÓDICO
        // ===================================================
        melodias_update();
    }
}