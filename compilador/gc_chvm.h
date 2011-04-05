#ifndef GC_CHVM_H_
#define GC_CHVM_H_

#include <iostream>
#include "tokens.h"
#include "parser.h"
#include "types.h"
#include "ch_asm.h"

using namespace std;

typedef struct {
	unsigned short segmentoCodigo;
	unsigned short segmentoDatos;
	FILE *file;
	TABLA_SIMBOLOS* simbolos;
	unsigned short direcciones[100]; //cuantos IF's, WHILE's, FOR's anidados soporta
	int direccionesCount;
} PROGRAMA;

void generar_codigo_chvm(PRODUCCION* ch, TABLA_SIMBOLOS* simbolos, string salida);

PROGRAMA* ch_crear(string nombre, TABLA_SIMBOLOS* simbolos);
void ch_escribir_encabezado(PROGRAMA* programa);
void ch_cerrar(PROGRAMA* programa);
void ch_escribir_tamanio_codigo(PROGRAMA* programa);
void ch_embedd_code_address(PROGRAMA* programa, short direccion);
void ch_embedd_code_address_in_placeholder(PROGRAMA* programa, short direccion);
void ch_embedd_code_address_placeholder(PROGRAMA* programa);
void ch_embedd_data_address(PROGRAMA* programa, PRODUCCION *etiqueta);
void ch_insertar_instruccion(PROGRAMA* programa, char codigo_instruccion);
void ch_embedd_string(PROGRAMA* programa, TOKEN *strings);
void ch_embedd_char(PROGRAMA* programa, char c);
void ch_embedd_integer(PROGRAMA* programa, int i);
void ch_embedd_double(PROGRAMA* programa, double d);

void ch_generar_ch(PRODUCCION *ch, PROGRAMA* programa);
void ch_generar_hijos(PRODUCCION *produccion_padre, PROGRAMA* programa);
void ch_generar_produccion(PRODUCCION *produccion, PROGRAMA* programa);

void ch_generar_produccion_asignacion(PRODUCCION *produccion, PROGRAMA* programa);
void ch_generar_produccion_lectura(PRODUCCION *produccion, PROGRAMA* programa);
void ch_generar_produccion_escritura(PRODUCCION *produccion, PROGRAMA* programa);
void ch_generar_produccion_if(PRODUCCION *produccion, PROGRAMA* programa);
void ch_generar_produccion_while(PRODUCCION *produccion, PROGRAMA* programa);
void ch_generar_produccion_for(PRODUCCION *produccion, PROGRAMA* programa);
void ch_generar_produccion_etiqueta(PRODUCCION *produccion, PROGRAMA* programa);
void ch_generar_produccion_expresion_string(PRODUCCION *produccion, PROGRAMA* programa);
void ch_generar_produccion_expresion_string_rr(PRODUCCION *produccion, PROGRAMA* programa);
void ch_generar_produccion_expresion_numero(PRODUCCION *produccion, PROGRAMA* programa);
void ch_generar_produccion_expresion_numero_rr(PRODUCCION *produccion, PROGRAMA* programa);
void ch_generar_produccion_termino(PRODUCCION *produccion, PROGRAMA* programa);
void ch_generar_produccion_termino_rr(PRODUCCION *produccion, PROGRAMA* programa);
void ch_generar_produccion_factor(PRODUCCION *produccion, PROGRAMA* programa);
void ch_generar_produccion_condicion(PRODUCCION *produccion, PROGRAMA* programa);
void ch_generar_produccion_condicion_rr(PRODUCCION *produccion, PROGRAMA* programa);
void ch_generar_produccion_factor_condicion(PRODUCCION *produccion, PROGRAMA* programa);
void ch_generar_produccion_string(PRODUCCION *produccion, PROGRAMA* programa);

#endif /*GC_CHVM_H_*/
